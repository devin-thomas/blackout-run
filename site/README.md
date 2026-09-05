# BLACKOUT RUN showcase

This dependency-free static site is deployed as a Cloudflare Worker at
[blackout-run.uppercut-labs.workers.dev](https://blackout-run.uppercut-labs.workers.dev).

All gameplay images are native 240x160 mGBA framebuffer captures displayed at 4x with nearest-neighbor scaling. Run `make site` from the repository root to refresh the ROM and captures.

Deploy with:

```sh
npx wrangler deploy --config site/wrangler.jsonc
```
