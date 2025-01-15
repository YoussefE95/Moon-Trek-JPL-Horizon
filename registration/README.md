## Build Image
```bash
docker build --tag=ubuntu-opencv .
```

## Create and Run Container
```bash
docker run -it \
    -u "$(id -u):$(id -g)" \
    -v $(pwd):/home/younix/Registration \
    -v $(pwd)/../server/static/results/:/home/younix/Images \
    --name moontrek-opencv ubuntu-opencv
```

## Run container
```bash
docker start -i moontrek-opencv
```