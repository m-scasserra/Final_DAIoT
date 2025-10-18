const config = require("./../config")
const path = require("path");
const glob = require("glob");

const registerRoutes = (router) => {
    const isWin = process.platform === "win32";

    // Only scan inside this file's folder
    const basePath = config.ENVIRONMENT === "dev" ? __dirname : config.ROUTER_PATH;

    // Resolve absolute path
    const absolutePath = path.resolve(basePath)
    
    // Glob for router directories (*.router/index.js pattern)
    const routes = glob.sync(path.join(absolutePath, "*.router/index.js"))

    routes.forEach(routePath => register(routePath, router));
}

const register = (routePath, router) => {
    const route = require(routePath);
    // Extract router name from "path/to/name.router/index.js" -> "name"
    const routerDir = routePath.split("/").slice(-2)[0]; // Get "name.router"
    const routeName = routerDir.split(".")[0]; // Get "name" from "name.router"
    console.log("Rutas registradas: ", routeName);
    router.use(`/${routeName}`, route.register(router));
}

module.exports = registerRoutes;
