# SQL Migration Guide for prueba.router

## What Was Changed

I've created SQL-compatible versions of your models and updated the `prueba.router` to work with MySQL instead of MongoDB.

## New Files Created

1. **`routers/prueba.router/models/dispositivos-sql.js`** - SQL wrapper for dispositivos table
2. **`routers/prueba.router/models/logs-sql.js`** - SQL wrapper for logs table  
3. **`sql/tables.sql`** - Database schema creation script
4. **`scripts/init-db.js`** - Database initialization script

## Modified Files

1. **`routers/prueba.router/index.js`** - Updated to use SQL models instead of MongoDB
2. **`storage/database/mysql.js`** - Enhanced with additional methods: `find`, `findOne`, `findOneAndUpdate`, `distinct`, `sort`
3. **`package.json`** - Added `init-db` npm script

## Database Schema

The SQL tables mirror your MongoDB collections:

### `dispositivos` table:
- `id` (auto-increment primary key)
- `dispositivoId` (device ID from IoT device)
- `nombre` (device name)
- `ubicacion` (location)
- `luz1`, `luz2` (light sensors)
- `temperatura`, `humedad` (temperature & humidity)
- `topic`, `topicSrvResponse` (MQTT topics)
- `created_at`, `updated_at` (timestamps)

### `logs` table:
- `id` (auto-increment primary key) 
- `logId` (sequential log ID)
- `ts` (timestamp)
- `eluz1`, `eluz2` (light values)
- `etemperatura`, `ehumedad` (sensor readings)
- `nodoId` (references device)
- `created_at` (timestamp)

## How to Use

1. **Ensure MySQL is enabled in `index.js`:**
   ```javascript
   // Make sure this is uncommented:
   require("./storage/database/mysql");
   // Make sure MongoDB is commented out:
   //require('./storage/database/mongo');
   ```

2. **Configure MySQL credentials in `.env`:**
   ```bash
   DB_MYSQL_USERNAME=your_username
   DB_MYSQL_PASSWORD=your_password
   DB_MYSQL_DBNAME=your_database
   DB_MYSQL_HOST=localhost
   DB_MYSQL_PORT=3306
   ```

3. **Initialize the database tables:**
   ```bash
   npm run init-db
   ```

4. **Start the application:**
   ```bash
   npm run dev
   ```

## Key Changes Made

1. **Replaced MongoDB operations:**
   - `dispositivo.find().distinct()` → `dispositivo.find()` + iteration
   - `logs.find().sort().limit()` → `logs.getMaxLogId()`
   - `findOneAndUpdate()` now uses SQL syntax
   - `_id` field references changed to `id`

2. **Added SQL-compatible methods:**
   - `find()`, `findOne()`, `findOneAndUpdate()`
   - `getMaxLogId()` for sequential log ID generation
   - `save()` method on model instances (Mongoose-like)

3. **Maintained API compatibility:**
   - Same REST endpoints: `/prueba/dispositivos`, `/prueba/status`
   - Same MQTT message processing logic
   - Same JSON response formats

## Testing

The router should now work exactly like before but with MySQL backend:

1. **MQTT messages** will create/update devices and logs in MySQL
2. **API endpoints** will return data from MySQL:
   - `GET /prueba/status` → `{"status": 200}`
   - `GET /prueba/dispositivos` → List all devices
   - `GET /prueba/dispositivos/:id` → Get specific device

3. **Device auto-creation** still works when MQTT messages arrive for new devices

The system maintains the same behavior but now uses relational database storage instead of MongoDB.