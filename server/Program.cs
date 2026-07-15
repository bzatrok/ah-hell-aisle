// AH: Hell Aisle — room relay.
//
// A deliberately dumb WebSocket router: it knows rooms, member identity and who
// the host is, and it moves opaque game frames between browsers. Everything the
// game means by those frames lives in the client (web/index.html + the wasm sim);
// nothing about weapons, positions or monsters is ever parsed here, so gameplay
// changes never require a server deploy.
//
//   GET /ws?room=CODE&name=NAME&color=N   → WebSocket
//
//   relay → client   {"t":"welcome","id":i,"hostId":h,"roster":[{id,name,color}]}
//                    {"t":"peer","id":i,"name":n,"color":c}      someone joined
//                    {"t":"left","id":i,"hostId":h}              someone left
//                    {"t":"g","from":i,"d":{...}}                a game frame
//   client → relay   {"t":"g","d":{...}}                         broadcast to others
//                    {"t":"g","to":i,"d":{...}}                  one recipient
//
// The host is always the oldest surviving member; the relay re-elects on every
// departure and the "left" notice carries the result. The host runs the monsters —
// the clients care who it is, the relay only keeps the answer consistent.

using System.Collections.Concurrent;
using System.Net.WebSockets;
using System.Text;
using System.Text.Json;

const int kMaxFrameBytes = 64 * 1024;   // snapshots are a few KB; anything bigger is abuse

var builder = WebApplication.CreateBuilder(args);

// Containers set ASPNETCORE_HTTP_PORTS (the aspnet base image uses 8080); a bare
// `dotnet run` on a dev machine gets the port the web client's fallback expects.
if (Environment.GetEnvironmentVariable("ASPNETCORE_URLS") is null &&
    Environment.GetEnvironmentVariable("ASPNETCORE_HTTP_PORTS") is null)
{
    builder.WebHost.UseUrls("http://0.0.0.0:8787");
}

var app = builder.Build();
app.UseWebSockets(new WebSocketOptions { KeepAliveInterval = TimeSpan.FromSeconds(30) });

var rooms = new ConcurrentDictionary<string, Room>();

app.MapGet("/healthz", () => Results.Ok(new { rooms = rooms.Count }));

app.Map("/ws", async context =>
{
    if (!context.WebSockets.IsWebSocketRequest)
    {
        context.Response.StatusCode = StatusCodes.Status400BadRequest;
        await context.Response.WriteAsync("websocket endpoint");
        return;
    }

    var code = Sanitize(context.Request.Query["room"], fallback: "AH");
    var name = Sanitize(context.Request.Query["name"], fallback: "KLANT");
    var color = int.TryParse(context.Request.Query["color"], out var c) ? Math.Clamp(c, 0, 7) : 0;

    var socket = await context.WebSockets.AcceptWebSocketAsync();

    // GetOrAdd can hand back a room that is concurrently tearing itself down
    // (last member left, dictionary removal pending) — retry until we land in
    // a live one or learn the room is genuinely full.
    Room room;
    Room.JoinResult? join;
    do
    {
        room = rooms.GetOrAdd(code, k => new Room());
        join = room.TryJoin(name, color, socket);
    } while (join is null && room.Closed);

    if (join is null)
    {
        await socket.CloseAsync(WebSocketCloseStatus.PolicyViolation, "room full",
                                CancellationToken.None);
        return;
    }

    var me = join.Me;

    // Roster and announce targets were snapshotted inside the join lock: everyone
    // already in my roster never hears "peer" about me, everyone who joins later
    // announces themselves to me — each side learns of the other exactly once.
    await me.SendAsync(JsonSerializer.Serialize(
        new { t = "welcome", id = me.Id, hostId = join.HostId, roster = join.Roster }));
    var announce = JsonSerializer.Serialize(
        new { t = "peer", id = me.Id, name = me.Name, color = me.Color });
    await Task.WhenAll(join.Others.Select(m => m.SendAsync(announce)));

    try
    {
        await Pump(room, me, socket, context.RequestAborted);
    }
    catch (Exception) when (socket.State != WebSocketState.Open)
    {
        // A vanished peer (tab closed, phone locked) surfaces as a socket
        // exception mid-receive; departure handling below is all it needs.
    }
    finally
    {
        var hostId = room.Leave(me);
        if (room.Closed) rooms.TryRemove(code, out _);
        else await room.BroadcastAsync(JsonSerializer.Serialize(
            new { t = "left", id = me.Id, hostId }), except: me.Id);
    }
});

app.Run();

// Reads frames from one member and routes them. Only the envelope is parsed —
// the "d" payload is forwarded verbatim via GetRawText.
static async Task Pump(Room room, Member me, WebSocket socket, CancellationToken ct)
{
    var buffer = new byte[8 * 1024];
    var frame = new MemoryStream();

    while (socket.State == WebSocketState.Open)
    {
        frame.SetLength(0);
        WebSocketReceiveResult result;
        do
        {
            result = await socket.ReceiveAsync(buffer, ct);
            if (result.MessageType == WebSocketMessageType.Close) return;
            frame.Write(buffer, 0, result.Count);
            if (frame.Length > kMaxFrameBytes)
            {
                await socket.CloseAsync(WebSocketCloseStatus.MessageTooBig, "frame too big", ct);
                return;
            }
        } while (!result.EndOfMessage);

        if (result.MessageType != WebSocketMessageType.Text) continue;

        string forward;
        int? to = null;
        try
        {
            using var doc = JsonDocument.Parse(frame.GetBuffer().AsMemory(0, (int)frame.Length));
            var root = doc.RootElement;
            if (!root.TryGetProperty("d", out var d)) continue;
            if (root.TryGetProperty("to", out var t) && t.TryGetInt32(out var toId)) to = toId;

            // The relay stamps the sender itself: "from" is the one field a
            // client must not be able to forge.
            forward = $"{{\"t\":\"g\",\"from\":{me.Id},\"d\":{d.GetRawText()}}}";
        }
        catch (JsonException)
        {
            continue;   // garbage in, nothing out
        }

        if (to is int target) await room.SendToAsync(target, forward);
        else await room.BroadcastAsync(forward, except: me.Id);
    }
}

static string Sanitize(string? value, string fallback)
{
    if (string.IsNullOrWhiteSpace(value)) return fallback;
    var cleaned = new string(value.Trim()
        .Where(ch => !char.IsControl(ch) && ch != '"' && ch != '\\')
        .Take(12).ToArray());
    return cleaned.Length == 0 ? fallback : cleaned.ToUpperInvariant();
}

sealed class Member(int id, string name, int color, WebSocket socket)
{
    public int Id { get; } = id;
    public string Name { get; } = name;
    public int Color { get; } = color;

    private readonly WebSocket _socket = socket;
    private readonly SemaphoreSlim _sendGate = new(1, 1);   // SendAsync forbids overlap

    public async Task SendAsync(string json)
    {
        var bytes = Encoding.UTF8.GetBytes(json);
        await _sendGate.WaitAsync();
        try
        {
            if (_socket.State == WebSocketState.Open)
                await _socket.SendAsync(bytes, WebSocketMessageType.Text, true, CancellationToken.None);
        }
        catch (Exception)
        {
            // A peer that died mid-send cleans itself up through its own pump.
        }
        finally
        {
            _sendGate.Release();
        }
    }
}

sealed class Room
{
    public const int MaxSize = 8;

    public sealed record JoinResult(Member Me, int HostId, object[] Roster, Member[] Others);

    public bool Closed { get; private set; }

    private readonly List<Member> _members = [];   // insertion order — [0] is the host
    private readonly object _gate = new();
    private int _nextId = 1;

    public JoinResult? TryJoin(string name, int color, WebSocket socket)
    {
        lock (_gate)
        {
            if (Closed || _members.Count >= MaxSize) return null;
            var others = _members.ToArray();
            var m = new Member(_nextId++, name, color, socket);
            _members.Add(m);
            return new JoinResult(m, _members[0].Id, RosterLocked(), others);
        }
    }

    private object[] RosterLocked() =>
        _members.Select(m => new { id = m.Id, name = m.Name, color = m.Color })
                .ToArray<object>();

    /// Removes the member and returns the surviving host id (0 for an empty,
    /// now-closed room). Closing under the same lock keeps a racing TryJoin
    /// from slipping into a room the dictionary is about to drop.
    public int Leave(Member m)
    {
        lock (_gate)
        {
            _members.Remove(m);
            if (_members.Count == 0)
            {
                Closed = true;
                return 0;
            }
            return _members[0].Id;
        }
    }

    public Task BroadcastAsync(string json, int except)
    {
        Member[] targets;
        lock (_gate) targets = _members.Where(m => m.Id != except).ToArray();
        return Task.WhenAll(targets.Select(m => m.SendAsync(json)));
    }

    public Task SendToAsync(int id, string json)
    {
        Member? target;
        lock (_gate) target = _members.FirstOrDefault(m => m.Id == id);
        return target?.SendAsync(json) ?? Task.CompletedTask;
    }
}
