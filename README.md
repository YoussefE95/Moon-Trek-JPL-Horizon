## Moon Trek with JPL Horizons
- Revisiting Moon Trek project [(Previous Repository)](https://github.com/YoussefE95/Moon-Trek-CSULA)
- Using JPL Horizons to get position and oriention of Earth and Moon, relative to the Sun

### Build ubuntu-opencv image
- This is required for the image registration. That tool is written in c++ and 
  uses opencv libraries which aren't included in apt repositories. This
  container will compile opencv including those libraries and will include all
  needed packages for compiling the registration tool.
```bash
docker build --tag=ubuntu-opencv ./Dockerfile.opencv
```

### Run both with compose
```bash
docker compose up
```

### Run client independently
```bash
cd client

docker build --tag=moontrek-client .

docker run -it --rm \
    -u $(id -u):$(id -g) \
    -v $(pwd):/usr/src/moontrek-client \
    -p 5173:5173 \
    --name moontrek-client moontrek-client
```

### Run server independently
```bash
cd server

docker build --tag=moontrek-server .

docker run -it --rm \
    -u $(id -u):$(id -g) \
    -v $(pwd):/usr/src/moontrek-server \
    -p 8890:8890 \
    --name moontrek-server moontrek-server
```
