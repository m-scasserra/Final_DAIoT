const db = require('./../../../storage/database/mysql');

class Dispositivos {
    static tableName = 'dispositivos';

    static async find(conditions = {}) {
        return await db.find(this.tableName, conditions);
    }

    static async findOne(conditions) {
        return await db.findOne(this.tableName, conditions);
    }

    static async findOneAndUpdate(conditions, updateData) {
        return await db.findOneAndUpdate(this.tableName, conditions, updateData);
    }

    static async create(data) {
        return await db.insert(this.tableName, data);
    }

    static async distinct(field, conditions = {}) {
        return await db.distinct(this.tableName, field, conditions);
    }

    // Save method for new instances (mimics Mongoose)
    constructor(data) {
        this.data = data;
    }

    async save() {
        return await db.insert(Dispositivos.tableName, this.data);
    }
}

module.exports = Dispositivos;