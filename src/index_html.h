#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <pgmspace.h>

const char page_setup[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
    <style>
        .IndexBody {
            display: flex;
            background-color: #F6F8FC;
            width: 98%;
            height: 98vh;
            flex-direction: column;
            font-family: arial;
        }

        .Header {
            display: flex;
            width: 100%;
            height: 80px;
            box-sizing: border-box;
            align-items: center;
            padding: 0px 80px 0px 80px;
        }

        .BoxShadow {
            background-color: #FAFAFA;
            display: flex;
            box-sizing: border-box;
            padding: 10px 11px 10px 11px;
            width: 432px;
            height: 570px; 
            flex-direction: column;
            border: 2px;
            box-shadow: 2px 2px 12px 0.25px #c0c7d9;
            border-radius: 36px;
            align-self: center;
        }

        .TitlePage,
        h1 {
            display: flex;
            justify-content: center;
            align-self: center;
            text-align: center;
            font-size: 24px;
            font-weight: 700;
            width: 204px;
            color: #39496D;
        }

        .FormBody {
            display: flex;
            flex-direction: column;
            font-size: 14px;
            font-weight: 400;
            justify-self: center;
            padding: 20px 40px 20px 40px;
        }

        .FormBody label {
            display: flex;
            margin-top: 5px;
            padding: 5px;
            color: #39496D;
            font-weight: 400;

        }

        .FormBody input {
            border-radius: 12.36px;
            box-sizing: border-box;
            width: 328px;
            height: 40px;
            background-color: #EDF0F4;
            border: 0px;
            margin: 5px;
            padding: 16px;
        }

        .SubmitButton {
            display: flex;
            justify-content: center;
        }

        .SubmitButton input {
            display: flex;
            width: 221px;
            height: 48px;
            align-self: center;
            justify-content: center;
            font-size: 16px;
            font-weight: 400;
            background-color: #265FFF;
            color: white;
            margin-top: 30px;
        }

        .Logo {
            display: flex;
            width: 200px;
            height: 65px;
        }
    </style>

    <body class="IndexBody">
        <div class="Header">
            <div class="Logo">

                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 549 44">
                    <defs>
                        <style>
                            .cls-1 {
                                fill: #150035;
                            }

                            .cls-2 {
                                fill: #265fff;
                            }

                            .cls-3 {
                                fill: #fff;
                            }
                        </style>
                    </defs>
                    <g id="Camada_2" data-name="Camada 2">
                        <g id="Camada_1-2" data-name="Camada 1">
                            <path class="cls-1"
                                d="M153.45,37.55A64.47,64.47,0,0,1,160.82,36a63.23,63.23,0,0,1,10.05-.73,23,23,0,0,1,8.67,1.42,13.31,13.31,0,0,1,5.55,4A15.13,15.13,0,0,1,188,46.83a34.17,34.17,0,0,1,.85,7.9V78.55h-9.8V56.27a32,32,0,0,0-.45-5.8,10.27,10.27,0,0,0-1.46-3.89,5.8,5.8,0,0,0-2.75-2.19,11.82,11.82,0,0,0-4.26-.68,33.12,33.12,0,0,0-3.89.24c-1.35.16-2.35.3-3,.4v34.2h-9.81Z" />
                            <path class="cls-1"
                                d="M239.21,57.32a27.46,27.46,0,0,1-1.46,9.16,20.36,20.36,0,0,1-4.13,7,18.6,18.6,0,0,1-6.44,4.53,20.81,20.81,0,0,1-8.31,1.62,20.5,20.5,0,0,1-8.26-1.62,18.77,18.77,0,0,1-6.41-4.53,20.92,20.92,0,0,1-4.17-7,26.71,26.71,0,0,1-1.5-9.16A26.36,26.36,0,0,1,200,48.2a20.49,20.49,0,0,1,4.22-7,18.48,18.48,0,0,1,6.44-4.49,20.61,20.61,0,0,1,8.18-1.58,21,21,0,0,1,8.23,1.58,18.12,18.12,0,0,1,6.44,4.49,21,21,0,0,1,4.17,7A26.36,26.36,0,0,1,239.21,57.32Zm-10.05,0q0-6.32-2.71-10a9.62,9.62,0,0,0-15.15,0q-2.73,3.69-2.72,10t2.72,10.13a9.56,9.56,0,0,0,15.15,0Q229.17,63.71,229.16,57.32Z" />
                            <path class="cls-1"
                                d="M287,77.25a56,56,0,0,1-7.33,1.62,56.57,56.57,0,0,1-9.28.73,24.6,24.6,0,0,1-9-1.54,18.58,18.58,0,0,1-6.77-4.41,19.37,19.37,0,0,1-4.29-7,26.62,26.62,0,0,1-1.5-9.2,30.94,30.94,0,0,1,1.25-9.12,20,20,0,0,1,3.69-7,16.35,16.35,0,0,1,5.91-4.5,19.18,19.18,0,0,1,8-1.58,18.25,18.25,0,0,1,5.43.73,23.69,23.69,0,0,1,4.05,1.62V17.29l9.8-1.62Zm-28.12-20q0,6.48,3.08,10.17a10.49,10.49,0,0,0,8.51,3.68,32.7,32.7,0,0,0,4-.2,26.32,26.32,0,0,0,2.72-.44V46.14a15,15,0,0,0-3.45-1.66,13.25,13.25,0,0,0-4.57-.77q-5.36,0-7.82,3.64T258.92,57.24Z" />
                            <path class="cls-1"
                                d="M296.7,57.56a26.54,26.54,0,0,1,1.66-9.8,20.55,20.55,0,0,1,4.42-7,18.18,18.18,0,0,1,6.32-4.22,19.61,19.61,0,0,1,7.29-1.41q8.75,0,13.65,5.42T335,56.75c0,.54,0,1.15,0,1.82s-.07,1.29-.13,1.83H306.83a11.2,11.2,0,0,0,3.6,7.9q3.21,2.79,9.28,2.79a30.6,30.6,0,0,0,6.52-.64,27.77,27.77,0,0,0,4.66-1.38l1.3,8a14.71,14.71,0,0,1-2.23.85,29.4,29.4,0,0,1-3.24.81c-1.22.24-2.53.45-3.93.61a38,38,0,0,1-4.29.24,26,26,0,0,1-9.73-1.66,17.92,17.92,0,0,1-6.8-4.62,18.87,18.87,0,0,1-4-7A28.54,28.54,0,0,1,296.7,57.56Zm28.44-4.37a12.81,12.81,0,0,0-.57-3.85,9.25,9.25,0,0,0-1.66-3.16,7.87,7.87,0,0,0-2.67-2.11,8.6,8.6,0,0,0-3.77-.77,8.73,8.73,0,0,0-4,.85,9.1,9.1,0,0,0-2.88,2.23,10.2,10.2,0,0,0-1.82,3.16,17.07,17.07,0,0,0-.89,3.65Z" />
                            <path class="cls-1" d="M374.1,78.55h-9.81v-38h9.81Z" />
                            <path class="cls-1"
                                d="M424.44,57.32A27.46,27.46,0,0,1,423,66.48a20.38,20.38,0,0,1-4.14,7,18.6,18.6,0,0,1-6.44,4.53,20.81,20.81,0,0,1-8.31,1.62,20.5,20.5,0,0,1-8.26-1.62,18.63,18.63,0,0,1-6.4-4.53,20.78,20.78,0,0,1-4.18-7,26.71,26.71,0,0,1-1.49-9.16,26.36,26.36,0,0,1,1.49-9.12,20.65,20.65,0,0,1,4.22-7,18.57,18.57,0,0,1,6.44-4.49,20.61,20.61,0,0,1,8.18-1.58,21,21,0,0,1,8.23,1.58,18.12,18.12,0,0,1,6.44,4.49,21,21,0,0,1,4.17,7A26.36,26.36,0,0,1,424.44,57.32Zm-10,0q0-6.32-2.72-10a9.62,9.62,0,0,0-15.15,0q-2.71,3.69-2.72,10t2.72,10.13a9.56,9.56,0,0,0,15.15,0Q414.4,63.71,414.4,57.32Z" />
                            <path class="cls-1" d="M471.21,22.39v8.84H454V78.55H443.66V31.23H426.4V22.39Z" />
                            <rect class="cls-2" x="363.6" y="22.39" width="11.19" height="11.19" rx="3.46" />
                            <path class="cls-2"
                                d="M79.29,0H35.46A35.46,35.46,0,0,0,0,35.46V79.3a35.45,35.45,0,0,0,35.46,35.45H79.29A35.46,35.46,0,0,0,114.75,79.3V35.46A35.47,35.47,0,0,0,79.29,0Z" />
                            <path class="cls-3"
                                d="M94.56,55V76.79H26a21.4,21.4,0,0,1-3.14-8A20.11,20.11,0,0,1,22.53,65a20.91,20.91,0,0,1,.41-4.14A21.66,21.66,0,0,1,44.21,43.32a22,22,0,0,1,3.37.26V52a13.07,13.07,0,0,0-3.39-.44A13.39,13.39,0,0,0,31.35,68.82H86.28V55a13.4,13.4,0,0,0-13.4-13.39H64.15V60.86H55.86V33.32h17A21.54,21.54,0,0,1,86.28,38a20.9,20.9,0,0,1,3.64,3.64A21.55,21.55,0,0,1,94.56,55Z" />
                        </g>
                    </g>
                </svg>
            </div>
        </div>

        <div class="BoxShadow">

            <div class="TitlePage">
                <h1>Configure seu <br> Wi-Fi e sua conta</h1>
            </div>

            <div class="FormBody">
                <form id="device-form">
                    <label for="ssid">Nome da rede Wi-Fi</label>
                    <input type="text" id="ssid" name="ssid" required>
                    <label for="password">Senha:</label>
                    <input type="text" id="password" name="password" required>
                    <label for="companyName">Nome da empresa:</label>
                    <input type="text" id="companyName" name="companyName" required>
                    <label for="deviceId">Nome do dispositivo:</label>
                    <input type="text" id="deviceId" name="deviceId">

                    <div class="SubmitButton">
                        <input type="submit" id="submitButton" value="Salvar">
                    </div>
                </form>
            </div>

        </div>

        <script>

            document.getElementById('device-form').addEventListener('submit', async e => {
                e.preventDefault();
                const new_ssid = document.getElementById('ssid').value;
                const new_password = document.getElementById('password').value;
                const new_companyName = document.getElementById('companyName').value;
                const new_deviceId = document.getElementById('deviceId').value;

                const deviceData = {
                    ssid: new_ssid,
                    password: new_password,
                    companyName: new_companyName,
                    deviceId: new_deviceId
                };

                const response = await fetch('/device-config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(deviceData)
                });
                
                const message = await response.text();
                console.log(message);
            });

        </script>
    </body>
</html>
)rawliteral";

const char page_setup_fail[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Node IoT - Rede Wi-Fi conectada.</title>
        <style>
            body {
                margin: 0;
                font-family: inter, sans-serif;
                background-color: #F6F8FC; /* Cor de fundo */
                display: flex;
                flex-direction: column;
                align-items: center;
                justify-content: center;
                min-height: 100vh;
            }
            header {
                width: 100%;
                height: 80px;
                background-color: #F6F8FC; /* Cor de fundo do header */
                display: flex;
                align-items: center;
                padding: 0 80px;
                position: fixed;
                top: 0;
                left: 0;
            }
            header img {
                height: 40px;
            }
            .card-container {
                display: flex;
                align-items: center;
                justify-content: center;
                width: 100%;
                height: calc(100vh - 80px); /* Ajuste para header fixo */
                margin-top: 80px;
            }
            .card {
                width: 90%;
                max-width: 430px;
                height: 90%;
                max-height: 378px;
                background-color: #ffffff;
                box-shadow: 0 4px 8px #6e799941;
                border-radius: 36px;
                display: flex;
                flex-direction: column;
                align-items: center;
                justify-content: center;
                padding: 20px;
                margin-bottom: 120px;
                text-align: center;
            }
            .icon {
                width: 70px;
                height: 70px;
                background-color: #F24A46;
                border-radius: 21px;
                display: flex;
                align-items: center;
                justify-content: center;
                color: white;
                font-size: 40px;
                font-weight: 600;
                margin-bottom: 20px;
            }
            .title {
                font-size: 24px;
                font-weight: bold;
                margin-bottom: 20px;
                margin-top: 10px;
                color: #F24A46;
            }
            .message {
                font-size: 18px;
                font-weight: 400;
                color: #39496D;
            }
        </style>
    </head>
    <body>
        <header>
            <img src="logo-node-iot.png" alt="Logo Node IoT">
        </header>
        <script>
            setTimeout(function(){ window.location.href = '/'; }, 5000);
        </script>
        <div class="card-container">
            <div class="card">
                <div class="icon">X</div>
                <div class="title">Falha na conexão.</div>
                <div class="message">Verifique as credenciais de rede
                    <br>e tente novamente.<br/> </div>
            </div>
        </div>
    </body>
</html>
)rawliteral";

const char page_setup_success[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Node IoT - Rede Wi-Fi conectada.</title>
        <style>
            body {
                margin: 0;
                font-family: inter, sans-serif;
                background-color: #F6F8FC; /* Cor de fundo */
                display: flex;
                flex-direction: column;
                align-items: center;
                justify-content: center;
                min-height: 100vh;
            }
            header {
                width: 100%;
                height: 80px;
                background-color: #F6F8FC; /* Cor de fundo do header */
                display: flex;
                align-items: center;
                padding: 0 80px;
                position: fixed;
                top: 0;
                left: 0;
            }
            header img {
                height: 40px;
            }
            .card-container {
                display: flex;
                align-items: center;
                justify-content: center;
                width: 100%;
                height: calc(100vh - 80px); /* Ajuste para header fixo */
                margin-top: 80px;
            }
            .card {
                width: 90%;
                max-width: 430px;
                height: 90%;
                max-height: 378px;
                background-color: #ffffff;
                box-shadow: 0 4px 8px #6e799941;
                border-radius: 36px;
                display: flex;
                flex-direction: column;
                align-items: center;
                justify-content: center;
                padding: 20px;
                margin-bottom: 120px;
                text-align: center;
            }
            .icon {
                width: 70px;
                height: 70px;
                background-color: #2AD0A0;
                border-radius: 21px;
                display: flex;
                align-items: center;
                justify-content: center;
                color: white;
                font-size: 40px;
                font-weight: 600;
                margin-bottom: 20px;
            }
            .title {
                font-size: 24px;
                font-weight: bold;
                margin-bottom: 20px;
                margin-top: 10px;
                color: #2AD0A0;
            }
            .message {
                font-size: 18px;
                font-weight: 400;
                color: #39496D;
            }
        </style>
    </head>
    <body>
        <header>
            <img src="logo-node-iot.png" alt="Logo Node IoT">
        </header>
        <script>
            setTimeout(function(){ window.location.href = '/'; }, 5000);
        </script>
        <div class="card-container">
            <div class="card">
                <div class="icon">✓</div>
                <div class="title">Conexão Wi-Fi realizada com sucesso!</div>
                <div class="message">Verifique a autenticação do seu dispositivo na plataforma NodeIoT.<br/> </div>
            </div>
        </div>
    </body>
</html>
)rawliteral";

#endif