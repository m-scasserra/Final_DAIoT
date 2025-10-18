const db = require('../../../storage/database/mysql');

class Logs {
    static tableName = 'logs';

    static async find(conditions = {}) {
        return await db.find(this.tableName, conditions);
    }

    static async findOne(conditions) {
        return await db.findOne(this.tableName, conditions);
    }

    static async create(data) {
        return await db.insert(this.tableName, data);
    }

    // Method to find and sort by logId descending with limit
    static async sort(sortObj) {
        if (sortObj.logId === -1) {
            return await db.sort(this.tableName, 'logId', 'DESC');
        }
        return await db.sort(this.tableName, 'logId', 'ASC');
    }

    constructor(data) {
        this.data = data;
    }

    async save() {
        return await db.insert(Logs.tableName, this.data);
    }
}

module.exports = Logs;