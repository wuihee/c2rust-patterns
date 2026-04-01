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
