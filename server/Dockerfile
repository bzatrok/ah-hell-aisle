FROM mcr.microsoft.com/dotnet/sdk:10.0 AS build
WORKDIR /src
COPY AhRelay.csproj .
RUN dotnet restore
COPY . .
RUN dotnet publish -c Release -o /out

FROM mcr.microsoft.com/dotnet/aspnet:10.0
WORKDIR /app
COPY --from=build /out .
# The aspnet base image sets ASPNETCORE_HTTP_PORTS=8080; compose maps it outward.
EXPOSE 8080
ENTRYPOINT ["dotnet", "AhRelay.dll"]
