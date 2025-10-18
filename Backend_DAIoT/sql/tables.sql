-- SQL table creation for dispositivos
CREATE TABLE IF NOT EXISTS dispositivos (
    id INT AUTO_INCREMENT PRIMARY KEY,
    mac_address VARCHAR(17) NOT NULL UNIQUE,
    nombre VARCHAR(255) NOT NULL,
    ubicacion VARCHAR(255),
    topic VARCHAR(500) NOT NULL,
    topicSrvResponse VARCHAR(500) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- SQL table creation for logs
CREATE TABLE IF NOT EXISTS logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    dispositivo_id INT NOT NULL,
    ts BIGINT NOT NULL,
    luz1 DECIMAL(10,2),
    luz2 DECIMAL(10,2),
    temperatura DECIMAL(10,2),
    humedad DECIMAL(10,2),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_dispositivo_id (dispositivo_id),
    INDEX idx_ts (ts),
    FOREIGN KEY (dispositivo_id) REFERENCES dispositivos(id) ON DELETE CASCADE
);