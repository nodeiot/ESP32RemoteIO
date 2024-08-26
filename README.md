<p align="center">
  <a href="https://nodeiot.com.br/"><img alt="NodeIoT" src="https://user-utilities.s3.amazonaws.com/assets/Documenta%C3%A7%C3%A3o/banner_repositorio.jpg" /></a>
</p>

# ESP32RemoteIO

Biblioteca para comunicação entre dispositivos [espressif32](https://www.espressif.com/en/products/socs/esp32) e plataforma em nuvem [NodeIoT](https://nodeiot.app.br/).

Possibilita a leitura de sensores e acionamento de atuadores em tempo real, com visualização de dados e ações de comando via dashboards web/mobile.

## Índice
- [ESP32RemoteIO](#esp32remoteio)
  - [Índice](#índice)
  - [Requisitos](#requisitos)
  - [Instalação](#instalação)
    - [Usando PlatformIO](#usando-platformio)
  - [Primeiro uso](#primeiro-uso)
    - [Passo a passo](#passo-a-passo)
  - [Documentação](#documentação)
  - [Classe RemoteIO](#classe-remoteio)
    - [Atributos](#atributos)
      - [setIO](#setio)
    - [Métodos](#métodos)
      - [begin](#begin)
      - [loop](#loop)
      - [updatePinOutput](#updatepinoutputstring-ref)
      - [espPOST](#esppoststring-variable-string-value)

## Requisitos
- Dispositivo [espressif32](https://www.espressif.com/en/products/socs/esp32).
- [PlatformIO](http://platformio.org).
- Conta [NodeIoT](https://nodeiot.app.br/register).
- Última versão estável do [esp32 Arduino core](https://github.com/espressif/arduino-esp32) instalada em sua IDE ([PlatformIO](http://platformio.org)).

## Instalação

### Usando PlatformIO
[PlatformIO](http://platformio.org) é um ecossistema de código aberto para o desenvolvimento de IoT com sistema de construção multiplataforma, gerenciador de bibliotecas e suporte completo para o desenvolvimento utilizando [Espressif](https://www.espressif.com/) ESP8266/ESP32. Funciona nos sistemas operacionais populares: Mac OS X, Windows, Linux 32/64, Linux ARM (como Raspberry Pi, BeagleBone, CubieBoard).

1. Instale o [PlatformIO IDE](http://platformio.org/platformio-ide).
2. Crie um novo projeto usando "PlatformIO Home > New Project".
3. Adicione "RemoteIO" ao projeto usando o [Project Configuration File `platformio.ini`](http://docs.platformio.org/page/projectconf.html) e o parâmetro [lib_deps](http://docs.platformio.org/page/projectconf/section_env_library.html#lib-deps):

```ini
[env:myboard]
platform = espressif32
board = ...
framework = arduino

# latest stable version
lib_deps = gkraemer-niot/ESP32RemoteIO
```

## Primeiro uso

Com os passos abaixo você poderá fazer a primeira interação de um hardware [espressif32](https://www.espressif.com/en/products/socs/esp32) com a plataforma [NodeIoT](https://nodeiot.app.br/).

### Passo a passo

- [Registre-se](https://nodeiot.app.br/register) na plataforma [NodeIoT](https://nodeiot.app.br), conforme [tutorial](https://nodeiot.com.br/docs/create-account).  
- Guarde o "Nome da instituição" definido durante seu registro. Ele será necessário para a configuração inicial do seu dispositivo.
- [Acesse](https://nodeiot.com.br/docs/create-account#accessing-the-platform) sua conta em: [NodeIoT](https://nodeiot.app.br).
- [Crie](https://nodeiot.com.br/docs/create-account#create-a-device) um dispositivo novo. 
- Como sugestão, adicione ao seu dispositivo uma variável para controlar, se disponível, o led embutido (built-in) no PCB do seu hardware. Defina uma referência (Ref) para facilitar a identificação dessa variável no firmware (caso deseje desenvolver outras operações com ela), além de um nome (Variável) para visualização na plataforma. Nas configurações da variável, selecione o modo OUTPUT e informe o pino que controla o led (usualmente o pino 2, verifique documentação da fabricante para mais informações). Salve as configurações do dispositivo.
- Copie o código do exemplo quickStart.ino abaixo e cole no arquivo principal do projeto em sua IDE. 
```ini
#include <ESP32RemoteIO.h>

RemoteIO device1;

void setup() 
{
  device1.begin();
}

void loop() 
{
  device1.loop();
}
```
- Faça upload do código para seu dispositivo.
- Reinicie o dispositivo para executar o código transferido.
- Em seu computador, verifique os pontos de acesso Wi-Fi disponíveis e conecte-se em "RemoteIO". Não é necessário senha.
- Após se conectar ao ponto de acesso, abra o browser de sua preferência (ex. Google Chrome, Mozilla Firefox) e acesse a URL: [http://192.168.4.1/](http://192.168.4.1/)
- Na tela de configurações, insira as credenciais (nome e senha) da rede Wi-Fi que será utilizada para conexão do dispositivo. Abaixo, informe o "Nome da instituição" definido por você em seu registro [NodeIoT](https://nodeiot.app.br/register). Por último, informe o nome do dispositivo criado dentro da plataforma e clique em Salvar. 
- Atente-se para preencher corretamente as informações acima solicitadas.
- Desconecte-se do ponto de acesso "RemoteIO" e conecte à rede de sua preferência.
- Acesse sua conta [NodeIoT](https://nodeiot.app.br/).
- Faça a [autenticação](https://nodeiot.com.br/docs/create-account#auth) do seu dispositivo.
- Crie um [dashboard](https://nodeiot.com.br/docs/create-account#dashboards) para interagir com seu dispositivo físico através de um painel na plataforma.
- Utilize as [ferramentas de dashboard](https://nodeiot.com.br/docs/create-account#tools) para inserir um "[Botão On/Off]()". Configure-o para trabalhar com o dispositivo e variável do led, criados por você. Personalize o visual, se desejar.
- Abra o [dashboard](https://nodeiot.app.br/dashboards) do seu dispositivo e utilize os componentes para interagir com o hardware RemoteIO. 

## Documentação

A [documentação NodeIoT](https://nodeiot.com.br/docs) apresenta informações mais detalhadas para uso da ferramenta, onde estão disponíveis guias e exemplos para aprender a trabalhar com as entradas e saídas do dispositivo, integrando o meio físico à núvem para construir a solução ideal para seu projeto.

## Classe RemoteIO

### Atributos

#### setIO

Neste objeto ficam armazenadas as configurações do dispositivo, recebidas após a autenticação. 

Por exemplo, se no [Passo a passo](#passo-a-passo) você tiver criado um dispositivo com uma configuração semelhante a essa: 
```ini
  Ref             Variável                Configurações
  led            led do PCB                OUTPUT     2
```
Você poderá obter informações do setIO da seguinte maneira:
```ini
  String mode = setIO["led"]["Mode"];               // mode = "OUTPUT"
  int ledPin = setIO["led"]["pin"].as<int>();       // ledPin = 2
  int ledValue = setIO["led"]["value"].as<int>();   // ledValue = X            (obtém o último valor armazenado para a variável, depende das interações feitas no sistema)
```

### Métodos

#### begin()

Inicializa o objeto RemoteIO e configura suas funcionalidades. Deve ser usado no setup do seu firmware para o correto funcionamento da comunicação entre dispositivo e NodeIoT.

#### loop()

Verifica as condições atuais do sistema e dispara ações conforme necessidade. Deve ser usado no loop do seu firmware para o correto funcionamento da comunicação entre dispositivo e NodeIoT.

#### updatePinOutput(String ref)

Atualiza, se configurado, o pino físico ligado à variável indicada pelo parâmetro "ref".

Exemplo:
```ini
  setIO["led"]["value"] = 1;   // representa o estado lógico HIGH
  
  updatePinOutput("led");      // coloca o pino 2 em estado lógico HIGH, ligando o led
```

#### espPOST(String variable, String value)

Envia à plataforma um novo valor "value" para a variável "variable".

Se em seu dispositivo na NodeIoT você tiver uma variável cuja referência(Ref) é: sensor_temperatura

Exemplo:
```ini
  espPOST("sensor_temperatura", "22.4");    // atualiza o valor que mostra a temperatura na plataforma
```
Com isso, se você tiver um componente tipo "Display" em seu dashboard, ligado à Ref "sensor_temperatura", verá o valor 22.4 sendo atualizado.
