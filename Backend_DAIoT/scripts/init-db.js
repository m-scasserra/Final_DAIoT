const mysql = require('mysql2');
const fs = require('fs');
const config = require('../config');

const DB_ENV = config.services.DATABASE.MYSQL;

async function initializeTables() {
    const connection = mysql.createConnection({
        host: DB_ENV.HOST,
        user: DB_ENV.USERNAME,
        password: DB_ENV.PASSWORD,
        database: DB_ENV.DBNAME,
        port: DB_ENV.PORT,
        multipleStatements: true
    });

    try {
        console.log('Initializing SQL tables...');
        
        const sqlScript = fs.readFileSync('./sql/tables.sql', 'utf8');
        
        await new Promise((resolve, reject) => {
            connection.query(sqlScript, (error, results) => {
                if (error) {
                    console.error('Error creating tables:', error);
                    reject(error);
                } else {
                    console.log('Tables created successfully');
                    resolve(results);
                }
            });
        });
        
    } catch (error) {
        console.error('Database initialization failed:', error);
    } finally {
        connection.end();
    }
}

// Run if called directly
if (require.main === module) {
    initializeTables();
}

module.exports = { initializeTables };