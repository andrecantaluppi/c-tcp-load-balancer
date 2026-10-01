#!/bin/bash

# 1. Ricompila il programma per sicurezza
echo "🔧 Compilazione del Load Balancer in corso..."
gcc -o balancer balancer.c -lpthread

# 2. Crea tre finte pagine web per distinguere i server
echo "🌐 Preparazione dei tre server backend finti..."
mkdir -p /tmp/backend1 /tmp/backend2 /tmp/backend3
echo "<html><body style='font-family:sans-serif; text-align:center; margin-top:50px;'><h1>🚀 Ciao dal Server 1</h1><p>In ascolto sulla porta 8001</p></body></html>" > /tmp/backend1/index.html
echo "<html><body style='font-family:sans-serif; text-align:center; margin-top:50px;'><h1>🚀 Ciao dal Server 2</h1><p>In ascolto sulla porta 8002</p></body></html>" > /tmp/backend2/index.html
echo "<html><body style='font-family:sans-serif; text-align:center; margin-top:50px;'><h1>🚀 Ciao dal Server 3</h1><p>In ascolto sulla porta 8003</p></body></html>" > /tmp/backend3/index.html

# 3. Avvia i server Python in background
python3 -m http.server 8001 --directory /tmp/backend1 > /dev/null 2>&1 &
P1=$!
python3 -m http.server 8002 --directory /tmp/backend2 > /dev/null 2>&1 &
P2=$!
python3 -m http.server 8003 --directory /tmp/backend3 > /dev/null 2>&1 &
P3=$!

# Spegni i server python automaticamente quando chiudi questo script
trap "echo ''; echo '🛑 Spegnimento dei server finti...'; kill $P1 $P2 $P3 2>/dev/null; exit" INT TERM EXIT

sleep 1
echo ""
echo "========================================================="
echo "✅ TUTTO PRONTO!"
echo "Apri Safari o Chrome e vai a questo indirizzo:"
echo "👉  http://127.0.0.1:8080"
echo ""
echo "Ricarica la pagina più volte per vedere il Round-Robin!"
echo "Premi CTRL+C qui nel terminale per fermare tutto."
echo "========================================================="
echo ""

# 4. Avvia il balancer e mostra i suoi log
./balancer
