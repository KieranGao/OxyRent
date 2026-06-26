FROM docker.cnb.cool/oxythecrack/docker-sync/node:20-alpine_amd64 AS build
WORKDIR /app
RUN npm config set registry https://registry.npmmirror.com
COPY package.json ./
RUN rm -f package-lock.json && npm install
COPY . .
RUN npm run build

FROM docker.cnb.cool/oxythecrack/docker-sync/nginx:alpine_amd64
COPY --from=build /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
