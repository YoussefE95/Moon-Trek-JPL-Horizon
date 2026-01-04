FROM node:latest

WORKDIR /usr/src/moontrek-client

COPY . .

RUN npm install

EXPOSE 5173

CMD [ "npm", "run", "dev" ]
