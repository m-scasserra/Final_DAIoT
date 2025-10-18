require("dotenv").config();

module.exports = {
    services: {
        API: {
            HOST: process.env.API_HOST || "",
            PORT: Number(process.env.API_PORT) || 3000
        },
        MQTT: {
            USERNAME: process.env.MQTT_USERNAME || "",
            PASSWORD: process.env.MQTT_PASSWORD || "",
            HOST: process.env.MQTT_HOST,
            PORT: Number(process.env.MQTT_PORT) || 8883,
            CA_CERT: process.env.MQTT_CA_CERT || "",
            CLIENT_CERT: process.env.MQTT_CLIENT_CERT || "",
            CLIENT_KEY: process.env.MQTT_CLIENT_KEY || ""
        },
        DATABASE: {
            MONGO: {
                USERNAME: process.env.DB_MONGO_USERNAME || "",
                PASSWORD: process.env.DB_MONGO_PASSWORD || "",
                DBNAME: process.env.DB_MONGO_NAME || "",
                HOST: process.env.DB_MONGO_HOST || "",
                PORT: Number(process.env.DB_MONGO_PORT) || 27017
            },
            MYSQL: {
                USERNAME: process.env.DB_MYSQL_USERNAME || "",
                PASSWORD: process.env.DB_MYSQL_PASSWORD || "",
                DBNAME: process.env.DB_MYSQL_DBNAME || "",
                HOST: process.env.DB_MYSQL_HOST || "",
                PORT: Number(process.env.DB_MYSQL_PORT) || 3306
            }
        }
    },
    ROUTER_PATH: process.env.ROUTER_PATH || "",
    ENVIRONMENT: process.env.ENVIRONMENT || ""
}