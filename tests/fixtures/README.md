# Canonical Fixture Catalog

This directory defines the canonical fixture corpus used by baseline CI and local
CI-parity runs.

## Taxonomy

- `x86/` - curated x86 fixture binaries
- `x64/` - curated x64 fixture binaries
- `arm/` - curated ARM fixture binaries

Top-level grouping is always architecture-first.

## Curated Core Policy

The baseline fixture set is intentionally small and representative. Do not treat
this directory as an exhaustive corpus dump.

When adding fixtures:

1. Add the `.bin` file to the correct architecture folder.
2. Add or update metadata in `manifest.yaml`.
3. Include rationale in the PR checklist.

## Metadata Contract

`manifest.yaml` is the source of truth for expected fixture behavior and ownership.
Each fixture entry must define:

- `fixture_id`
- `arch`
- `path`
- `expected_outcome`
- `owner`
- `notes`

## Ownership and Review

Fixture changes must be reviewable and attributable. Every fixture set change
requires:

- updated `manifest.yaml` metadata
- a PR rationale describing why the fixture was added, changed, or removed
