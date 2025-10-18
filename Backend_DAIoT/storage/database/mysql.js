const mysql = require("mysql2");
const config = require("./../../config");
const DB_ENV = config.services.DATABASE.MYSQL;
const dbConfig = {
    host: DB_ENV.HOST,
    user: DB_ENV.USERNAME,
    password: DB_ENV.PASSWORD,
    database: DB_ENV.DBNAME,
    port: DB_ENV.PORT
};

let connection;

function handleConnection() {
    connection = mysql.createConnection(dbConfig);
    connection.connect((err) => {
        if (err) {
            console.error("[db-error]", err);
            setTimeout(handleConnection, 2000);
        } else console.log("DB CONNECTED");
    });

    connection.on("error", (err) => {
        console.error("[db-error]", err);
        if (err.code === "PROTOCOL_CONNECTION_LOST") {
            handleConnection();
        } else {
            throw err;
        }
    });
}

handleConnection();

function list(table) {
    return new Promise((resolve, reject) => {
        connection.query(`SELECT * FROM ${table}`, (error, data) => {
            if (error) return reject(error);
            resolve(data);
        });
    });
}

function get(table, id) {
    return new Promise((resolve, reject) => {
        connection.query(
            `SELECT * FROM ${table} WHERE id='${id}'`,
            (error, data) => {
                if (error) return reject(error);
                resolve(data);
            }
        );
    });
}

function insert(table, data) {
    return new Promise((resolve, reject) => {
        connection.query(`INSERT INTO ${table} SET ?`, data, (error, result) => {
            if (error) return reject(error);
            resolve(result);
        });
    });
}

function update(table, data) {
    return new Promise((resolve, reject) => {
        connection.query(
            `UPDATE ${table} SET ? WHERE id=?`,
            [data, data.id],
            (error, result) => {
                if (error) return reject(error);
                resolve(result);
            }
        );
    });
}

function upsert(table, data) {
    if (data && data.id) return update(table, data);
    return insert(table, data);
}

function query(table, query, join = null) {
    return new Promise((resolve, reject) => {
        let joinQuery = "";

        if (join) {
            const key = Object.keys(join)[0];
            const val = join[key];
            joinQuery = `JOIN ${key} ON ${table}.${val} = ${key}.id`;
        }

        const sql = connection.query(
            `SELECT * FROM ${table} ${joinQuery} WHERE ?`,
            query,
            (error, result) => {
                if (error) return reject(error);
                console.log(result);
                resolve(JSON.parse(JSON.stringify(result[0] || {})) || null);
            }
        );
        console.log("Sql", sql.sql)
    });
}


function find(table, options = {}) {
    return new Promise((resolve, reject) => {
        let sql = `SELECT * FROM ${table}`;
        const values = [];

        // WHERE
        if (options.where && Object.keys(options.where).length > 0) {
            const whereClauses = Object.keys(options.where)
                .map(key => `${key} = ?`)
                .join(' AND ');
            sql += ` WHERE ${whereClauses}`;
            values.push(...Object.values(options.where));
        }

        // ORDER BY
        if (options.order && Array.isArray(options.order)) {
            const [field, direction] = options.order[0];
            sql += ` ORDER BY ${field} ${direction}`;
        }

        // LIMIT
        if (options.limit) {
            sql += ` LIMIT ${options.limit}`;
        }

        connection.query(sql, values, (error, result) => {
            if (error) return reject(error);
            resolve(result);
        });
    });
}


function findOne(table, conditions) {
    return new Promise((resolve, reject) => {
        const conditionKeys = Object.keys(conditions);
        const conditionString = conditionKeys.map(key => `\`${key}\` = ?`).join(' AND ');
        const values = Object.values(conditions);

        const sql = `SELECT * FROM \`${table}\` WHERE ${conditionString} LIMIT 1`;

        connection.query(sql, values, (error, result) => {
            if (error) return reject(error);
            resolve(result[0] || null);
        });
    });
}


function findOneAndUpdate(table, conditions, updateData) {
    return new Promise((resolve, reject) => {
        connection.query(
            `UPDATE ${table} SET ? WHERE ?`,
            [updateData, conditions],
            (error, result) => {
                if (error) return reject(error);
                // Return the updated record
                connection.query(`SELECT * FROM ${table} WHERE ?`, conditions, (err, data) => {
                    if (err) return reject(err);
                    resolve(data[0] || null);
                });
            }
        );
    });
}

function distinct(table, field, conditions = {}) {
    return new Promise((resolve, reject) => {
        let sql = `SELECT DISTINCT ${field} FROM ${table}`;
        let params = [];
        
        if (Object.keys(conditions).length > 0) {
            sql += ' WHERE ?';
            params.push(conditions);
        }
        
        connection.query(sql, params, (error, result) => {
            if (error) return reject(error);
            resolve(result.map(row => row[field]));
        });
    });
}

function sort(table, sortField, order = 'ASC', limit = null) {
    return new Promise((resolve, reject) => {
        let sql = `SELECT * FROM ${table} ORDER BY ${sortField} ${order}`;
        if (limit) sql += ` LIMIT ${limit}`;
        
        connection.query(sql, (error, result) => {
            if (error) return reject(error);
            resolve(result);
        });
    });
}

const createDocument = (model, params) => {
    return new model({ ...params }).save()
}

module.exports = {
    list,
    get,
    upsert,
    query,
    find,
    findOne,
    findOneAndUpdate,
    distinct,
    sort,
    insert
};