FROM docker.cnb.cool/oxythecrack/docker-sync/node:20-alpine_amd64
WORKDIR /app
COPY package*.json ./
RUN npm install --registry=https://registry.npmmirror.com
COPY . .
EXPOSE 50056
CMD ["node", "server.js"]
