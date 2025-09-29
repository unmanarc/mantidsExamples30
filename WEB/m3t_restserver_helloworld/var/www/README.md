# Managing Web Resource Dependencies


## Update Packages
This command updates the specified npm packages (jquery, bootstrap, fontawesome) to their latest versions available in the registry, modifying the package-lock.json and updating the node_modules directory accordingly.

```bash
npm update jquery bootstrap @fortawesome/fontawesome-free
```

## Install files into the project

This command executes the build process defined in the project's package.json file, which typically compiles, processes, and copies the downloaded resources into the appropriate directories for use in the application.

```bash
npm run build-assets
```
