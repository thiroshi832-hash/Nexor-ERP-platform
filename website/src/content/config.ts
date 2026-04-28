import { defineCollection } from 'astro:content';
import { docsSchema }      from '@astrojs/starlight/schema';

// Legacy-style content collection (Astro 4 + Starlight 0.30): the
// schema-only definition is enough.  Astro 5's content-layer API uses
// `loader: docsLoader()` from '@astrojs/starlight/loaders' instead.
export const collections = {
    docs: defineCollection({
        schema: docsSchema(),
    }),
};
