# Deploy Melvin Chat with Docker

## Quick Start

### 1. Build and Run Locally

```bash
docker-compose up --build
```

This will:
- Build the Docker image (compiles the C server)
- Start the container
- Make the site available at `http://localhost:8080`

### 2. Run in Background

```bash
docker-compose up -d --build
```

### 3. Stop the Server

```bash
docker-compose down
```

## Deploy to Cloud

### Option 1: Docker Hub + Any Cloud Provider

1. **Build and tag:**
   ```bash
   docker build -t yourusername/melvin-chat .
   ```

2. **Push to Docker Hub:**
   ```bash
   docker push yourusername/melvin-chat
   ```

3. **Deploy on any cloud provider:**
   - AWS ECS/Fargate
   - Google Cloud Run
   - Azure Container Instances
   - DigitalOcean App Platform
   - Railway
   - Render
   - Fly.io

### Option 2: Railway (Easiest)

1. Install Railway CLI: `npm i -g @railway/cli`
2. Login: `railway login`
3. Deploy: `railway up`
4. Your site will get a public URL automatically!

### Option 3: Render

1. Connect your GitHub repo to Render
2. Create a new "Web Service"
3. Use these settings:
   - **Build Command:** `docker build -t melvin .`
   - **Start Command:** `docker run -p 8080:8080 melvin`
   - **Port:** `8080`

### Option 4: Fly.io

1. Install Fly CLI: `flyctl install`
2. Login: `flyctl auth login`
3. Launch: `flyctl launch`
4. Deploy: `flyctl deploy`

### Option 5: DigitalOcean App Platform

1. Connect GitHub repo
2. Select "Docker" as build type
3. Auto-detects Dockerfile
4. Deploy!

## Environment Variables

You can customize the port:

```bash
docker run -p 3000:8080 -e PORT=8080 melvin-chat
```

## Persisting Brain State

To save/load brain state across container restarts:

1. Uncomment the volume in `docker-compose.yml`:
   ```yaml
   volumes:
     - ./brains:/app/brains
   ```

2. Modify `melvin_server.c` to save/load from `/app/brains/` directory

## Health Check

The server runs on port 8080. Check health:

```bash
curl http://localhost:8080/api/status
```

## Production Tips

1. **Use a reverse proxy** (nginx/traefik) for HTTPS
2. **Set resource limits** in docker-compose:
   ```yaml
   deploy:
     resources:
       limits:
         memory: 512M
   ```

3. **Use Docker secrets** for sensitive config
4. **Enable logging** for debugging

## Troubleshooting

- **Port already in use:** Change port in docker-compose.yml
- **Build fails:** Make sure gcc is available (included in Dockerfile)
- **Can't connect:** Check firewall/security groups allow port 8080

## Example: Deploy to Railway

```bash
# Install Railway CLI
npm i -g @railway/cli

# Login
railway login

# Initialize project
railway init

# Deploy
railway up
```

Railway will automatically:
- Build your Docker image
- Deploy it
- Give you a public URL like: `https://melvin-chat.railway.app`

## Example: Deploy to Render

1. Go to https://render.com
2. New → Web Service
3. Connect GitHub repo
4. Settings:
   - **Name:** melvin-chat
   - **Environment:** Docker
   - **Region:** Choose closest
5. Deploy!

You'll get: `https://melvin-chat.onrender.com`
