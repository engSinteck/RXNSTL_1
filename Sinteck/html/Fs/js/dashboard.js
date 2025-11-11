// dashboard.js - Lógica de comunicação com JSON

// --- FUNÇÕES DE COMUNICAÇÃO ---

// Mantido: Envia comando RF ON/OFF (mantém a comunicação GET original)
function sendData(status) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById("RFState").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "RFState="+status, true);
    xhttp.send();
}

// Polling: Loop de atualização de telemetria a cada 500ms
setInterval(function() {
    getTELEMETRY();
}, 500);

// Chamada inicial (inicia a cadeia de busca de dados)
function getData() {
    getLogin();
}

// Mantido: Busca de Login/Lock (Mantém o parsing de string original, se o backend não for JSON)
function getLogin() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            // Lógica de parsing de string original
            document.getElementById("Login").innerHTML = this.responseText.substring(this.responseText.indexOf("User:")+5,this.responseText.indexOf("Config:")-1);
            document.getElementById("lock").innerHTML = this.responseText.substring(this.responseText.indexOf("Config:")+7,this.responseText.indexOf("FIM:")-1);
            
            // Lógica de ícone de cadeado (fa-lock ou fa-unlock)
            const lockState = document.getElementById("lock").innerHTML;
            const lockIcon = (lockState === '0') ? '<i class="fa-solid fa-unlock"></i>' : '<i class="fa-solid fa-lock"></i>';
            document.getElementById("lock2").innerHTML = lockIcon;
            
			getTELEMETRY();
        }
    };
    xhttp.open("GET", "readLogin", true);
    xhttp.send();
}

// --- FUNÇÕES DE LÓGICA ---

function calculateVSWR(fwd, ref) {
    if (fwd === 0 || fwd < 0) return 99.99;
    var reflectionCoefficient = Math.sqrt(ref / fwd);
    var vswr = (1 + reflectionCoefficient) / (1 - reflectionCoefficient);
    return vswr;
}


// NOVO getTELEMETRY() usando JSON
function getTELEMETRY() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            let data;
            try {
                // A chave: Transforma a resposta JSON em um objeto JavaScript
                data = JSON.parse(this.responseText);
            } catch (e) {
                console.error("Erro ao parsear JSON. Verifique o output do STM32.", this.responseText);
                document.getElementById("STATUS-display").textContent = "JSON ERROR";
                return;
            }

            // 1. Cálculo de VSWR
            const vswr = calculateVSWR(data.FWD, data.REF);

            // 2. Telemetria Principal (Power Metrics)
            document.getElementById("metric-fwd-power").textContent = data.FWD.toFixed(0);
            document.getElementById("metric-ref-power").textContent = data.REF.toFixed(0);
            document.getElementById("metric-efficiency").textContent = data.EFIC.toFixed(1);
            document.getElementById("metric-vswr").textContent = vswr.toFixed(2);
            
            // 3. Temperaturas
            document.getElementById("temp-main").textContent = data.TEMP.toFixed(1) + ' °C';
			document.getElementById("c").textContent = data.TEMP.toFixed(1) + ' °C';
            
            // 4. Telemetria Geral
            document.getElementById("VPA-display").textContent = data.VPA.toFixed(2) + ' V';
            document.getElementById("IPA-display").textContent = data.IPA.toFixed(2) + ' A';
            document.getElementById("UPTIME-display").textContent = data.UPTIME;
            document.getElementById("FREQ-display").textContent = (data.FREQ / 1000).toFixed(3) + ' MHz';
            document.getElementById("AUDIO-display").textContent = data.AUDIO;
            document.getElementById("RDS-display").textContent = data.RDS;
            document.getElementById("STATUS-display").textContent = data.STS;
            document.getElementById("FALHA-display").textContent = data.FAIL;
            document.getElementById("last-update").textContent = data.CLOCK; 
            document.getElementById("MODEL").textContent = data.MODEL;
			document.getElementById("TOKEN-display").textContent = data.SERIAL;
            
            // 5. MPX Meters
            document.getElementById('BarMPX1').value = data.B1;
            document.getElementById('BarMPX2').value = data.B2;
            document.getElementById('BarMPX3').value = data.B3;
            document.getElementById("B1-value").textContent = data.B1;
            document.getElementById("B2-value").textContent = data.B2;
            document.getElementById("B3-value").textContent = data.B3;
        }
    };
    xhttp.open("GET", "readTELEMETRYJSON", true);
    xhttp.send();
}