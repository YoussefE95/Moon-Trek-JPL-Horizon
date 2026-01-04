FROM ubuntu-opencv:latest

RUN apt update && apt install -y npm nodejs

WORKDIR /usr/src/moontrek-server

COPY . .

RUN npm install

EXPOSE 8890

CMD [ "./start.sh" ]
