<script setup>
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls'
import * as THREE from 'three'
import { getTimezoneOffset } from 'date-fns-tz'
import tzloopup from '@photostructure/tz-lookup'
import parser from  'exif-parser'
import axios from 'axios'
import { onMounted, reactive } from 'vue'

const input = reactive({
    image: '',
    date: '',
    time: '',
    lat: 0,
    lon: 0,
})

const result = reactive({
    view: 'images',
    positions: {
        earth: {},
        moon: {}
    },
    model: {
        camera: {
            perspective: 'telescope',
            anchor: 'moon'
        },
        images: {
            real: '',
            layer: '',
        }
    },
    timestamp: ''
})

const scene = new THREE.Scene()
const camera = new THREE.PerspectiveCamera(45, 2000 / 2000, 1, 100000000)
const renderer = new THREE.WebGLRenderer({
    antialias: true,
    preserveDrawingBuffer: true
})
const orbit = new OrbitControls(camera, renderer.domElement)
const sun = new THREE.DirectionalLight(0xffffff, 2.5, 1000000)
const earth = new THREE.Mesh(
    new THREE.SphereGeometry(6371, 30, 30),
    new THREE.MeshPhongMaterial({
        map: new THREE.TextureLoader().load(
            'http://localhost:8890/moon-trek/images/textures/earth.jpg'
        ),
        shininess: 0
    })
)
const moon = new THREE.Mesh(
    new THREE.SphereGeometry(1737, 30, 30),
    new THREE.MeshPhongMaterial({
        map: new THREE.TextureLoader().load(
            'http://localhost:8890/moon-trek/images/textures/overlay.png'
        ),
        shininess: 0
    })
)
const person = new THREE.Mesh(
    new THREE.SphereGeometry(250, 30, 30),
    new THREE.MeshBasicMaterial({
        color: 0xFF0000
    })
)

const rectToCart = () => {
    const R = 6371
    const lat = -(input.lat * Math.PI) / 180
    const lon = -(input.lon * Math.PI) / 180

    return {
        x: R * Math.cos(lat) * Math.cos(lon),
        y: -R * Math.sin(lat),
        z: R * Math.cos(lat) * Math.sin(lon)
    }
}

const padVal = (val) => {
    return String(val).padStart(2, '0')
}

const inputImage = (event) => {
    if (event.target.files) {
        try {
            const reader = new FileReader();

            reader.onloadend = () => {
                input.image = reader.result
            }

            reader.readAsDataURL(event.target.files[0])
        } catch (error) {
            console.log("Couldn't read image")
        }

        try {
            const reader = new FileReader();

            reader.onloadend = () => {
                const { tags } = parser.create(reader.result).parse()

                if (tags.DateTimeOriginal) {
                    const date = new Date(tags.DateTimeOriginal * 1000)
                    input.date = `${date.getUTCFullYear()}-${padVal(date.getUTCMonth())}-${padVal(date.getUTCDate())}`
                    input.time = `${padVal(date.getUTCHours())}:${padVal(date.getUTCMinutes())}`
                } else {
                    console.log('No date EXIF data')
                }

                if (tags.GPSLatitude && tags.GPSLongitude) {
                    input.lat = tags.GPSLatitude
                    input.lon = tags.GPSLongitude
                } else {
                    console.log('No gps EXIF data')
                }
            }

            reader.readAsArrayBuffer(event.target.files[0])
        } catch (error) {
            console.log('No EXIF data')
        }
    }
}

const sendImages = async () => {
    const { data } = await axios({
        method: 'post',
        url: 'http://localhost:8890/moon-trek/registration',
        data: { 
            user: input.image,
            real: result.model.images.real,
            layer: result.model.images.layer
        }
    })

    result.timestamp = data.timestamp
}

const getDateRange = () => {
    const utcDate = new Date(`${input.date}T${input.time}`)
    utcDate.setMilliseconds(
        utcDate.getMilliseconds() +
        (-1.0 * getTimezoneOffset(tzloopup(input.lat, input.lon)))
    )

    const utcNext = new Date(utcDate)
    utcNext.setDate(utcDate.getDate() + 1)

    return {
        start: `${utcDate.getUTCFullYear()}-${utcDate.getUTCMonth()}-${utcDate.getUTCDate()}`,
        stop: `${utcNext.getUTCFullYear()}-${utcNext.getUTCMonth()}-${utcNext.getUTCDate()}`
    }
}

const getPositions = async () => {
    const { start, stop } = getDateRange()

    const { data } = await axios({
        method: 'get',
        url: 'http://localhost:8890/moon-trek/positions',
        params: {
            start,
            stop,
            index: Number(input.time.split(':')[0])
        }
    })

    result.positions.earth = data.earth
    result.positions.moon = data.moon
}

const setPositions = () => {
    earth.position.set(
        result.positions.earth.x,
        result.positions.earth.y,
        result.positions.earth.z
    )
    earth.rotation.x =
        (result.positions.earth.lat * Math.PI) / 180
    earth.rotation.y =
        (-1 * (180 + result.positions.earth.lon) * Math.PI) / 180

    moon.position.set(
        result.positions.moon.x,
        result.positions.moon.y,
        result.positions.moon.z
    )
    moon.lookAt(earth.position)
    moon.rotateOnAxis(
        new THREE.Vector3(0, 1, 0),
        (-90 * Math.PI) / 180 
    )
    moon.rotateOnAxis(
        new THREE.Vector3(1, 0, 0),
        (-1 * result.positions.moon.lat * Math.PI) / 180 
    )
    moon.rotateOnAxis(
        new THREE.Vector3(0, 1, 0),
        (-1 * (result.positions.moon.lon) * Math.PI) / 180
    )

    const personPos = rectToCart()
    person.position.set(
        personPos.x,
        personPos.y,
        personPos.z
    )

    person.getWorldPosition(camera.position)
    camera.lookAt(moon.position)
    camera.updateProjectionMatrix()

    sun.target = earth
}

const toggleView = () => {
    if (result.view === 'images') {
        result.view = 'model'
    } else {
        result.view = 'images'
    }
}

const toggleAnchor = () => {
    const newFocusPosition = {}

    if (result.model.camera.anchor === 'earth') {
        result.model.camera.anchor = 'moon'
        newFocusPosition.x = result.positions.moon.x 
        newFocusPosition.y = result.positions.moon.y 
        newFocusPosition.z = result.positions.moon.z
    } else {
        result.model.camera.anchor = 'earth'
        newFocusPosition.x = result.positions.earth.x
        newFocusPosition.y = result.positions.earth.y
        newFocusPosition.z = result.positions.earth.z
    }

    const newWorldBox = new THREE.Box3().setFromCenterAndSize(
        new THREE.Vector3(
            newFocusPosition.x,
            newFocusPosition.y,
            newFocusPosition.z
        ),
        new THREE.Vector3(0.1, 0.1, 0.1)
    )
    newWorldBox.getCenter(orbit.target)
    orbit.update()
}

const togglePerspective = () => {
    if (result.model.camera.perspective === 'telescope') {
        result.model.camera.perspective = 'orbit'
        camera.zoom = 1
    } else {
        result.model.camera.perspective = 'telescope'

        if (result.model.camera.anchor === 'earth') {
            toggleAnchor()
        }
        
        person.getWorldPosition(camera.position)
        camera.lookAt(moon.position)
        camera.zoom = 85
        camera.updateProjectionMatrix()
    }
}

const setTexture = async (texture) => {
    moon.material.map = await new THREE.TextureLoader().loadAsync(
        `http://localhost:8890/moon-trek/images/textures/${texture}`
    )
}

const toggleTexture = async () => {
    if (moon.material.map.source.data.currentSrc.includes("moon-2k.jpg")) {
        await setTexture("overlay.png")
    } else {
        await setTexture("moon-2k.jpg")
    }
}

const renderToDataUrl = () => {
    renderer.setSize(2000, 2000)
    camera.aspect = 1
    camera.updateProjectionMatrix()
    renderer.render(scene, camera)

    return renderer.domElement.toDataURL('image/png')
}

const sendIt = async () => {
    await getPositions()
    setPositions()

    await setTexture("overlay.png")
    result.model.images.layer = renderToDataUrl()

    await setTexture("moon-2k.jpg")
    result.model.images.real = renderToDataUrl()

    await sendImages()
}

onMounted(async () => {
    document.getElementById('model-canvas').appendChild(renderer.domElement)

    scene.add(sun)
    scene.add(earth)
    scene.add(moon)
    earth.add(person)

    camera.zoom = 85

    renderer.setAnimationLoop(async () => {
        const width = window.innerWidth
        const height = window.innerHeight * 0.9

        renderer.setSize(width, height)
        camera.aspect = width / height
        camera.updateProjectionMatrix()

        renderer.render(scene, camera)
    })
})
</script>

<template>
<main>
    <div class="row align-items-center">
        <div class="col text-center">
            <input id="fileInput" type="file" v-on:change="inputImage"/>
        </div>
        <div class="col text-center">
            <input type="date" v-model="input.date"/>
            <input type="time" v-model="input.time"/>
        </div>
        <div class="col text-center">
            <input type="number" v-model="input.lat"/>
            <input type="number" v-model="input.lon"/>
        </div>
        <div class="col text-center">
            <input type="button" value="Send it" @click="sendIt"/>
        </div>
    </div>
    <div class="row align-items-center">
        <div class="col text-center">
            <input type="button" value="Toggle view" @click="toggleView"/>
        </div>
    </div>
    <div v-if="result.view === 'model'">
        <div class="row align-items-center" v-if="result.positions.moon">
            <div class="col text-center">
                <input type="button" value="Toggle Texture" @click="toggleTexture"/>
            </div>
            <div class="col text-center">
                <input type="button" value="Toggle Perspective" @click="togglePerspective"/>
            </div>
            <div class="col text-center">
                <input type="button"
                value="Toggle Anchor"
                @click="toggleAnchor"
                :disabled="result.model.camera.perspective !== 'orbit'"/>
            </div>
        </div>
    </div>
    <div v-else>
        <div class="row align-items-center" v-if="result.model.images.real">
            <div class="col text-center">
                <img :src="result.model.images.real" />
            </div>
            <div class="col text-center">
                <img :src="result.model.images.layer" />
            </div>
        </div>
        <div v-if="result.timestamp">
            <div class="row align-items-center">
                <div class="col text-center">
                    <img :src="`http://localhost:8890/moon-trek/images/results/${result.timestamp}/cropped.png`" />
                </div>
                <div class="col text-center">
                    <img :src="`http://localhost:8890/moon-trek/images/results/${result.timestamp}/stacked.png`" />
                </div>
            </div>
            <div class="row align-items-center">
                <div class="col text-center">
                    <img :src="`http://localhost:8890/moon-trek/images/results/${result.timestamp}/detected-matches.png`" />
                </div>
            </div>
            <div class="row align-items-center">
                <div class="col text-center">
                    <img :src="`http://localhost:8890/moon-trek/images/results/${result.timestamp}/detected-circles.png`" />
                </div>
            </div>
        </div>
    </div>
    <div id="model-canvas" v-show="result.view === 'model'"></div>
</main>
</template>

<style scoped>
:global(body) {
  background-color: #282828;
}

#fileInput {
    color: white;
}

.row {
    margin: 1.25rem 0rem;
}

img {
    width: 100%
}
</style>