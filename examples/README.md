# Examples Layout

The examples directory is organized by pattern first, then validity, then source repository.

## Directory Structure

```text
examples/
├── manifests/
│   └── <pattern>.yaml
└── <pattern>/
    ├── valid/
    │   └── <repo>/
    │       └── <source>_<pattern>.c
    └── invalid/
        └── <repo>/
            └── <reason>.c
```

## Naming Conventions

- Valid snippets: `<source>_<pattern>.c`
- Invalid snippets: `<reason>.c`

## Manifest Conventions

Each manifest (`examples/manifests/<pattern>.yaml`) should include:

- `pattern`
- `required_components`
- `examples` entries with:
  - `id`
  - `status` (`valid` or `invalid`)
  - `repo`
  - `path`
  - `source`
  - `missing_components` (for invalid examples)

This structure keeps comparison and automated validation straightforward across repositories.
