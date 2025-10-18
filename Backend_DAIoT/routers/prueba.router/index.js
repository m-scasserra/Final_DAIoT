const dispositivo = require("./models/dispositivos-sql");
const logs = require("./models/logs-sql");
const clientMqtt = require("../../storage/mqtt");
const options = clientMqtt.MQTTOptions;
var arrayTopicsListen = ["/#"];


clientMqtt.on("connect", async function () {
    clientMqtt.subscribe(arrayTopicsListen, options, () => {
        console.log("Subscribed to topics: ");
        console.log(arrayTopicsListen);
    });
    const test_mensaje = {
        luz1: 0,
        luz2: 0,
        temperatura: 16,
        humedad: 80
    };
    const payload = JSON.stringify(test_mensaje);

    // Publico mensajes al inicio del servicio para verificar la subscripción
    var testTopic = "/AABBCCDDEEFF/data";
    clientMqtt.publish(testTopic, payload, options, (error) => {
        if (error) {
            console.log(error);
        }
    })

    clientMqtt.on("message", async (topic, payload) => {
        console.log("[MQTT] Mensaje recibido: " + topic + ": " + payload.toString());

        // Procesar el mensaje recibido
        mac = topic.split('/')[1]; // Extraer la MAC del topic
        console.log("MAC extraída del topic: " + mac);

        if (!mac) {
            console.log("TOPIC INCORRECTO, DEBE INICIAR CON /MAC_DEL_DISPOSITIVO/...");
            return;
        }

        if (topic !== `/${mac}/data`) {
            console.log("TOPIC INCORRECTO, DEBE TERMINAR EN /data");
            return;
        }

        // Verificar que el topic siga el formato esperado
        var mensaje = payload.toString();
        let json;
        try {
            json = JSON.parse(mensaje);
        } catch (error) {
            console.log("FORMATO INCORRECTO, DEBE ENVIAR MENSAJES EN FORMATO JSON");
            return; // Salir de la función en caso de error de formato
        }

        // Verificar la existencia de todos los campos
        const camposEsperados = ['luz1', 'luz2', 'temperatura', 'humedad'];
        const camposFaltantes = camposEsperados.filter((campo) => !(campo in json));
        if (camposFaltantes.length > 0) {
            console.log('CAMPOS FALTANTES: ', camposFaltantes.join(', '));
            return;
        }

        // Validar el formato del JSON
        if (typeof json.luz1 !== 'number' || typeof json.luz2 !== 'number' || typeof json.temperatura !== 'number' || typeof json.humedad !== 'number') {
            console.log('FORMATO INCORRECTO');
            return;
        }

        // Busco coincidencia de topic y nombre de dispositivo en la DB
        const buscarDispositivo = await dispositivo.findOne({
            mac_address: mac,
        });

        if (buscarDispositivo) { // Si el dispositivo existe agrego un log
            console.log("Dispositivo registrado, procedo a agregar un log.");
            const logToSave = new logs({
                dispositivo_id: buscarDispositivo.id,
                ts: new Date().getTime(),
                luz1: json.luz1,
                luz2: json.luz2,
                temperatura: json.temperatura,
                humedad: json.humedad,
            });
            console.log(logToSave);
            try {
                const savedLog = await logToSave.save();
                console.log("REGISTRO DE LOG AGREGADO CORRECTAMENTE.");
            } catch (error) {
                console.log("ERROR UPDATING");
            }
        } else { // Si no existe creo un nuevo dispositivo
            console.log("Dispositivo no registrado, procedo a crearlo.");
            console.log("Topic recibido: " + topic);
            console.log("Datos del dispositivo: ");
            console.log(json);
            // Agrego un nuevo dispositivo 
            const nuevodisp = new dispositivo({
                mac_address: mac,
                nombre: "New device",
                ubicacion: "Not located",
                topic: topic,
                topicSrvResponse: "/".concat(mac).concat("/cmd"),
            });
            console.log("Dispositivo nuevo creado ok");
            try {
                const savedDisp = await nuevodisp.save();
                console.log("NUEVO DISPOSITIVO AGREGADO CORRECTAMENTE.");
            } catch (error) {
                console.log("ERROR UPDATING");
            }
            const buscarDispositivo = await dispositivo.findOne({
                mac_address: mac,
            });
            const logToSave = new logs({
                dispositivo_id: buscarDispositivo.id,
                ts: new Date().getTime(),
                luz1: json.luz1,
                luz2: json.luz2,
                temperatura: json.temperatura,
                humedad: json.humedad,
            });
            console.log(logToSave);
            try {
                const savedLog = await logToSave.save();
                console.log("REGISTRO DE LOG AGREGADO CORRECTAMENTE.");
            } catch (error) {
                console.log("ERROR UPDATING");
            }
        }
    })

})

const register = (router) => {
    router.get("/status", (req, resp) => resp.json({ status: 200 }));

    router.get('/dispositivos', async function (req, res) {
        const listado = await dispositivo.find();
        if (!listado) return res.json({ data: null, error: 'No hay datos en la Base de Datos.' });
        if (listado) return res.json({ data: listado, error: null });
    });

    router.get('/dispositivos/:id', async function (req, res) {
        const listado = await dispositivo.findOne({ "id": req.params.id });
        if (!listado) return res.json({ data: null, error: 'No hay datos en la Base de Datos.' });
        if (listado) return res.json({ data: listado, error: null });
    });

    router.put('/dispositivos', async function (req, res) {
        const { id, nombre, ubicacion } = req.body;
        if (!id || !nombre || !ubicacion) {
            return res.status(400).json({ data: null, error: 'Faltan datos obligatorios: id, nombre, ubicacion' });
        }
        try {
            const updatedDisp = await dispositivo.findOneAndUpdate(
                { id: id },
                { nombre: nombre, ubicacion: ubicacion }
            );
            if (!updatedDisp) {
                return res.status(404).json({ data: null, error: 'Dispositivo no encontrado' });
            }
            return res.json({ data: updatedDisp, error: null });
        } catch (error) {
            return res.status(500).json({ data: null, error: 'Error al actualizar el dispositivo' });
        }
    });

    router.get('/logs/:id', async function (req, res) {
        const listado = await logs.find({ "dispositivo_id": req.params.id });

        if (!listado || listado.length === 0) {
            return res.json({
                data: null,
                error: 'No hay datos en la Base de Datos.'
            });
        }

        return res.json({
            data: listado,
            error: null
        });
    });

    router.get('/logs/:id/last30', async function (req, res) {
        console.log("Buscando los últimos 30 logs del dispositivo ID:", req.params.id);

        try {
            const listado = await logs.find({
                where: { dispositivo_id: req.params.id },
                order: [['id', 'DESC']],
                limit: 30
            });

            if (!listado || listado.length === 0) {
                return res.json({
                    data: null,
                    error: 'No hay datos en la Base de Datos.'
                });
            }

            res.json({ data: listado });
        } catch (err) {
            console.error("Error al consultar logs:", err);
            res.status(500).json({ error: err.message });
        }
    });


    router.get('/logs/:id/last', async function (req, res) {
        const log = await logs.findOne({
            where: { dispositivo_id: req.params.id },
            order: [['logId', 'DESC']],
        });

        if (!log) {
            return res.json({
                data: null,
                error: 'No hay datos en la Base de Datos.'
            });
        }
        return res.json({
            data: log,
            error: null
        });
    });

    router.post('/control/luz/:id', async function (req, res) {
        console.log(req.body);
        const { luz1 } = req.body;
        if (luz1 === undefined) {
            return res.status(400).json({ data: null, error: 'Faltan datos obligatorios: luz1' });
        }
        const buscarDispositivo = await dispositivo.findOne({ "id": req.params.id });
        if (!buscarDispositivo) {
            return res.status(404).json({ data: null, error: 'Dispositivo no encontrado' });
        }
        const topicToPublish = buscarDispositivo.topicSrvResponse;
        const payload = luz1 === 'on' ? 'LUZ ON' : 'LUZ OFF';
        clientMqtt.publish(topicToPublish, payload, options, (error) => {
            if (error) {
                console.log(error);
                return res.status(500).json({ data: null, error: 'Error al publicar el mensaje MQTT' });
            } else {
                console.log(`Mensaje publicado en el topic ${topicToPublish}: ${payload}`);
                return res.json({ data: 'Mensaje publicado correctamente', error: null });
            }
        });
    });

    router.post('/control/report/:id', async function (req, res) {
        const buscarDispositivo = await dispositivo.findOne({ "id": req.params.id });
        if (!buscarDispositivo) {
            return res.status(404).json({ data: null, error: 'Dispositivo no encontrado' });
        }
        const topicToPublish = buscarDispositivo.topicSrvResponse;
        const payload = "REPORT";
        clientMqtt.publish(topicToPublish, payload, options, (error) => {
            if (error) {
                console.log(error);
                return res.status(500).json({ data: null, error: 'Error al publicar el mensaje MQTT' });
            } else {
                console.log(`Mensaje publicado en el topic ${topicToPublish}: ${payload}`);
                return res.json({ data: 'Mensaje publicado correctamente', error: null });
            }
        });
    });

    return router;
};

module.exports = {
    register,
};
