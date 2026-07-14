# AH: Hell Aisle

A Doom 2 clone set in an Albert Heijn after closing time. Born as a four-model
bake-off (see `SPEC.md` / `JUDGING.md`); `claude/` won and is the game under
active development. `CLAUDE.md` has the full repo layout and build instructions.

Play it at <https://ah-hell-aisle.pages.dev>.

## Deploy

The web build is deployed manually to the Cloudflare Pages project
`ah-hell-aisle`. From the repo root:

```sh
./web/build.sh && npx wrangler pages deploy web/dist --project-name=ah-hell-aisle --branch=main
```

- `web/build.sh` compiles `claude/` to WASM with Emscripten (`brew install
  emscripten`) into `web/dist/`. First run also clones and builds raylib for
  the web; later runs reuse it.
- `--branch=main` is required: the Pages project's production branch is `main`
  (it predates the repo's rename to `master`), and without the flag a deploy
  from `master` lands as a preview instead of production.

### Deploy on commit (optional, currently off)

`.github/workflows/deploy.yml` can do the same build + deploy in CI but is
manual-only (`workflow_dispatch`). To enable deploy-on-commit:

1. Add a `push: branches: [master]` trigger to the workflow's `on:` block.
2. Set two repo secrets: `CLOUDFLARE_API_TOKEN` (token with *Account →
   Cloudflare Pages → Edit*) and `CLOUDFLARE_ACCOUNT_ID`.
