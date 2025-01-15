const axios = require('axios');

const getEphemeris = async (target, center, type, start, stop) => {
    const { data } = await axios({
        method: 'get',
        url: 'https://ssd.jpl.nasa.gov/api/horizons.api',
        params: {
            format: 'text',
            OBJ_DATA: 'NO',
            COMMAND: target,
            CENTER: center,
            EPHEM_TYPE: type,
            START_TIME: start,
            STOP_TIME: stop,
            STEP_SIZE: '60m',
            REF_PLANE: 'ECLIPTIC',
            REF_SYSTEM: 'ICRF',
            OUT_UNITS: 'KM-S',
            VEC_TABLE: '1',
            QUANTITIES: '14'
        }
    });

    return data;
};

const toUTC = (date, time) => {
    const months = [
        'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
        'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'
    ];
    const splitDate = date.split('-');
    const monthNum = String(
            months.findIndex(m => m === splitDate[1]) + 1
        ).padStart(2, '0');

    return `${splitDate[0]}-${monthNum}-${splitDate[2]}T${time.split('.')[0]}`;
};

const parse = (vector, observer) => {
    const vectorLines = vector.split('\n');
    const vectorStart = vectorLines.findIndex(l => l === '$$SOE') + 1;
    const vectorEnd = vectorLines.findIndex(l => l === '$$EOE') - 1;

    const observerLines = observer.split('\n');
    const observerStart = observerLines.findIndex(l => l === '$$SOE') + 1;
    const observerEnd = observerLines.findIndex(l => l === '$$EOE') - 1;

    const parsed = [];
    let i = vectorStart;
    let j = observerStart;
    while (i < vectorEnd && j < observerEnd) {
        const vectorDate = vectorLines[i].replace(/=/g, '').match(/[^ ]+/g);
        const vectorLine = vectorLines[i+1].replace(/=/g, '').match(/[^ ]+/g);
        const observerLine = observerLines[j].match(/[^ ]+/g);

        parsed.push({
            utc: toUTC(vectorDate[2], vectorDate[3]),
            x: Number(vectorLine[1]),
            y: Number(vectorLine[5]),
            z: -Number(vectorLine[3]),
            lon: Number(observerLine[2]),
            lat: Number(observerLine[3])
        });

        i += 2;
        j++;
    }

    return parsed;
};

module.exports = {
    getEphemeris, 
    parse
};
