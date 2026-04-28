# Nexor docs site

[Astro](https://astro.build) + [Starlight](https://starlight.astro.build)
docs site rendering the markdown under `src/content/docs/`. Source for
the published site at `https://nexor.io/docs/` (when deployed).

## Local development

```bash
cd website
npm install
npm run dev          # http://localhost:4321
```

## Build for production

```bash
npm run build        # writes static HTML to dist/
npm run preview      # serves dist/ locally for verification
```

## Layout

```
website/
  package.json
  astro.config.mjs            # Starlight integration + sidebar
  tsconfig.json
  public/
    logo.png                  # site logo (Nexor "N" mark)
    favicon.ico
  src/
    content.config.ts         # content collection schema
    styles/
      nexor.css               # tweaks to Starlight defaults
    content/
      docs/                   # every page mirrors a route under /
        index.mdx             # homepage / overview
        getting-started.md
        language-reference.md
        forms.md
        sheets.md
        processes.md
        publishing.md
        server-side.md
        api-reference.md
        samples/
          index.md
          hello-world.md
          customer-crud.md
          order-approval.md
```

## Updating content

The markdown under `src/content/docs/` is the source of truth for the
published docs. The `docs/` folder at the repo root is kept in sync — the
quickest workflow is:

1. Edit a page in `website/src/content/docs/<page>.md`
2. Mirror the change into `docs/<page>.md`
3. `npm run build && npm run preview` to verify locally

A future `tools/sync_docs.py` will collapse the two trees into one.
