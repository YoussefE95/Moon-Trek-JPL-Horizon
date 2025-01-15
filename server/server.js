const { getEphemeris, parse } = require('./ephemeris.js');
const util = require('util');
const exec = util.promisify(require('child_process').exec);
const fs = require('fs/promises');
const express = require('express');
const cors = require('cors');

const app = express();
const port = 8890;

app.use(express.json({
    limit: '10mb'
}));
app.use(cors());

app.use('/moon-trek/images', express.static('static'));

app.get('/moon-trek/positions', async (req, res) => {
    try {
        const { start, stop, index } = req.query;

        res.status(200).json({
            earth: parse(
                await getEphemeris(399, '@sun', 'VECTORS', start, stop),
                await getEphemeris(399, '@sun', 'OBSERVER', start, stop)
            )[index],
            moon: parse(
                await getEphemeris(301, '@sun', 'VECTORS', start, stop),
                await getEphemeris(301, '@399', 'OBSERVER', start, stop)
            )[index]
        });
    } catch (error) {
        res.status(500).json({ error });
    }
});

const writeDataUrl = async (name, dataUrl) => {
    const matches = dataUrl.match(/^data:([^;]+);base64,(.+)$/);
    const [, , base64Data] = matches;
    const buffer = Buffer.from(base64Data, 'base64');

    await fs.writeFile(name, buffer);
};
app.post('/moon-trek/registration', async (req, res) => {
    try {
        const { user, real, layer } = req.body;
        const timestamp = Date.now();
        let path = `./static/results/${timestamp}`;

        await fs.mkdir(path);

        await writeDataUrl(`${path}/user.png`, user);
        await writeDataUrl(`${path}/real.png`, real);
        await writeDataUrl(`${path}/layer.png`, layer);

        await exec(`docker exec moontrek-opencv ./build/main ${timestamp}`);

        res.status(200).json({
            message: 'Success',
            timestamp
        });
    } catch (error) {
        res.status(500).json({ error });
    }
});

app.listen(port, () => {
    console.log(`Sever is listening on port ${port}`);
});