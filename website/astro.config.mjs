// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// https://astro.build/config
// https://starlight.astro.build/reference/configuration/
//
// Note: we deliberately omit `site:` so Starlight's bundled @astrojs/sitemap
// integration becomes a no-op (it requires `site:` to function).  The
// sitemap is optional for the docs site and currently zod-incompatible on
// this Node release; once Starlight ships a version with a fixed peer
// constraint, restore `site: 'https://nexor.io'`.
export default defineConfig({
    integrations: [
        starlight({
            title: 'Nexor',
            description:
                'ERP platform built on a single VB-Script-flavored language ' +
                'and four desktop apps: Studio, Core, Command, Flux.',
            logo: {
                src: './public/logo.png',
                alt: 'Nexor',
            },
            favicon: '/favicon.ico',
            social: {
                github: 'https://github.com/your-org/nexor',
            },
            customCss: ['./src/styles/nexor.css'],

            // Nexor is VB-Script-flavored - alias `vbnet` and `nexor` to
            // Shiki's bundled `vb` grammar so the syntax-highlighter
            // doesn't fall back to plain text on our examples.
            expressiveCode: {
                shiki: {
                    langAlias: { vbnet: 'vb', nexor: 'vb' },
                },
            },

            sidebar: [
                {
                    label: 'Start here',
                    items: [
                        { label: 'Overview',        link: '/' },
                        { label: 'Getting started', slug: 'getting-started' },
                    ],
                },
                {
                    label: 'Authoring',
                    items: [
                        { label: 'Language reference', slug: 'language-reference' },
                        { label: 'Forms',              slug: 'forms' },
                        { label: 'Sheets & Entities',  slug: 'sheets' },
                        { label: 'Processes & BPMN',   slug: 'processes' },
                    ],
                },
                {
                    label: 'Operations',
                    items: [
                        { label: 'Publishing & Deployment', slug: 'publishing' },
                        { label: 'Server-side activities',  slug: 'server-side' },
                        { label: 'Core HTTP API reference', slug: 'api-reference' },
                    ],
                },
                {
                    label: 'Samples',
                    autogenerate: { directory: 'samples' },
                },
            ],
        }),
    ],
});
