<p align="center">
  <a href="https://nodeiot.com.br/"><img alt="NodeIoT" src="https://user-utilities.s3.amazonaws.com/assets/Documenta%C3%A7%C3%A3o/banner_repositorio.jpg" /></a>
</p>

# ESP32RemoteIO

Biblioteca para comunicação entre dispositivos [espressif32](https://www.espressif.com/en/products/socs/esp32) e plataforma em nuvem [NodeIoT](https://nodeiot.app.br/).

Possibilita a criação de aplicações em tempo real, leitura de sensores, acionamento de atuadores, visualização de dados e ações de comando via dashboards web/mobile.

## Índice
- [ESP32RemoteIO](#esp32remoteio)
  - [Índice](#índice)
  - [Requisitos](#requisitos)
  - [Instalação](#instalação)
  - [Primeiro uso](#primeiro-uso)
    - [Passo a passo](#passo-a-passo)
  - [Classe RemoteIO](#classe-remoteio)
    - [Atributos](#atributos)
      - [setIO](#setio)
    - [Métodos](#métodos)
      - [begin](#begin)
      - [loop](#loop)
      - [updatePinOutput](#updatepinoutputstring-ref)
      - [espPOST](#esppoststring-variable-string-value)
  - [Como construir suas próprias aplicações](#como-construir-suas-próprias-aplicações)
    - [Função de callback](#função-de-callback)
  - [Documentação](#documentação)

## Requisitos
- Dispositivo [espressif32](https://www.espressif.com/en/products/socs/esp32).
- [Microsoft's Visual Studio Code](https://code.visualstudio.com/).
- [PlatformIO](http://platformio.org).
- Conta [NodeIoT](https://nodeiot.app.br/register).
- Última versão estável do [esp32 Arduino core](https://github.com/espressif/arduino-esp32) instalada em sua IDE ([PlatformIO](http://platformio.org)).
- Bibliotecas necessárias: [ArduinoJson](https://github.com/bblanchon/ArduinoJson), [WebSockets](https://github.com/Links2004/arduinoWebSockets) e [ESPAsyncWebServer](https://github.com/mathieucarbou/ESPAsyncWebServer). 

## Instalação

1. Faça download e instale o [Microsoft's Visual Studio Code](https://code.visualstudio.com/).
2. Faça o download da biblioteca pelo link [ESP32Remoteio](https://github.com/nodeiot/ESP32RemoteIO/archive/refs/heads/master.zip).
3. Abra o VSCode e acesse, no menu lateral esquerdo, o Extensions Manager (Ctrl+Shift+X).
4. Procure pela extensão oficial do [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) e instale-a.
5. Crie um novo projeto em "PlatformIO Home > New Project". 
6. Descompacte o arquivo .zip da biblioteca ESP32RemoteIO em seu sistema.
7. Acesse os arquivos descompactados, procure a pasta 'src', selecione os arquivos dentro dela e copie-os (Ctrl+C).
8. Acesse o diretório do seu projeto do PlatformIO, cole os arquivos copiados (Ctrl+V) dentro da pasta 'include'.
9. Acesse o projeto criado via PlatformIO e abra o arquivo 'platformio.ini'.
10. Por último, adicione as seguintes dependências (bibliotecas) ao seu projeto por meio do parâmetro 'lib_deps', como mostrado abaixo:

```ini
[env:myboard]
platform = espressif32
board = ...
framework = arduino
lib_deps =
  bblanchon/ArduinoJson @ 7.1.0
	Links2004/WebSockets @ 2.4.2
	mathieucarbou/ESPAsyncWebServer @ 3.3.22
```

## Primeiro uso

Com os passos abaixo você poderá fazer a primeira interação de um hardware [espressif32](https://www.espressif.com/en/products/socs/esp32) com a plataforma [NodeIoT](https://nodeiot.app.br/).

### Passo a passo

- [Registre-se](https://nodeiot.app.br/register) na plataforma [NodeIoT](https://nodeiot.app.br), conforme [tutorial](https://nodeiot.com.br/docs/create-account).  
- Guarde o "Nome da instituição" definido durante seu registro. Ele será necessário para a configuração inicial do seu dispositivo.
- [Acesse](https://nodeiot.com.br/docs/create-account#accessing-the-platform) sua conta em: [NodeIoT](https://nodeiot.app.br).
- [Crie](https://nodeiot.com.br/docs/create-account#create-a-device) um dispositivo novo. 
- Como sugestão, adicione ao seu dispositivo uma variável para controlar, se disponível, o led embutido (built-in) no PCB do seu hardware. Defina uma referência (Ref) para facilitar a identificação dessa variável no firmware (caso deseje desenvolver outras operações com ela), além de um nome (Variável) para visualização na plataforma. Nas configurações da variável, selecione o modo OUTPUT e informe o pino que controla o led (usualmente o pino 2, verifique documentação da fabricante para mais informações). Salve as configurações do dispositivo.
- Copie o código do exemplo abaixo (quickStart.ino) e cole no arquivo principal do projeto em sua IDE. 

```ini
#include <ESP32RemoteIO.h>

RemoteIO device1;

/* 
  Define your own callback function to handle communication events with NodeIoT
  Example:
*/

void myCallback(String ref, String value)
{
  /* 
  if (ref == "turnOnLight")
  {
    device1.updatePinOutput(ref);        // This function will update the IO pin linked to 'turnOnLight' variable on previously done NodeIoT device configuration
  }
  */

  //Serial.printf("[myCallback] ref: %s, value: %s\n", ref, value);
}

void setup() 
{
  device1.begin(myCallback);
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

## Classe RemoteIO

### Atributos

#### setIO

Neste objeto ficam armazenadas as configurações do dispositivo, recebidas após a autenticação. 

Por exemplo, se no [Passo a passo](#passo-a-passo) você tiver criado um dispositivo com uma configuração semelhante a essa: 
```ini
  Ref             Variável                Configurações
  led            led do PCB               OUTPUT      2
```
Você poderá obter informações do setIO da seguinte maneira:

```ini
#include <ESP32RemoteIO.h>

RemoteIO device1;

/* 
  Define your own callback function to handle communication events with NodeIoT
  Example:
*/

void myCallback(String ref, String value)
{
  /* 
  if (ref == "turnOnLight")
  {
    device1.updatePinOutput(ref);        // This function will update the IO pin linked to 'turnOnLight' variable on previously done NodeIoT device configuration
  }
  */

  //Serial.printf("[myCallback] ref: %s, value: %s\n", ref, value);
}

void setup() 
{
  device1.begin(myCallback);

  String mode = device1.setIO["led"]["Mode"].as<String>();               // mode = "OUTPUT"
  int ledPin = device1.setIO["led"]["pin"].as<int>();       // ledPin = 2
  int ledValue = device1.setIO["led"]["value"].as<int>();   // ledValue = X            (obtém o último valor armazenado para a variável, depende das interações feitas no sistema)

}

void loop() 
{
  device1.loop();
}
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
#include <ESP32RemoteIO.h>

RemoteIO device1;

/* 
  Define your own callback function to handle communication events with NodeIoT
  Example:
*/

void myCallback(String ref, String value)
{
  //Serial.printf("[myCallback] ref: %s, value: %s\n", ref, value);
}

void setup() 
{
  device1.begin(myCallback);

  String mode = device1.setIO["led"]["Mode"].as<String>();               // mode = "OUTPUT"
  int ledPin = device1.setIO["led"]["pin"].as<int>();       // ledPin = 2
  int ledValue = device1.setIO["led"]["value"].as<int>();   // ledValue = X            (obtém o último valor armazenado para a variável, depende das interações feitas no sistema)

  // exemplo: ligar o led sempre que iniciar o dispositivo
  
  if (mode == "OUTPUT")
  {
    device1.setIO["led"]["value"] = "1";
    device1.updatePinOutput("led");                           // atualiza o estado do pino físico associado à variável "led"
  }

}

void loop() 
{
  device1.loop();
}
```

#### espPOST(String variable, String value)

Envia à plataforma um novo valor ("value") para a variável ("variable").

Se em seu dispositivo na NodeIoT você tiver uma variável cuja referência(Ref) é: led

Exemplo:
```ini
#include <ESP32RemoteIO.h>

RemoteIO device1;

/* 
  Define your own callback function to handle communication events with NodeIoT
  Example:
*/

void myCallback(String ref, String value)
{
  //Serial.printf("[myCallback] ref: %s, value: %s\n", ref, value);
}

void setup() 
{
  device1.begin(myCallback);

  String mode = device1.setIO["led"]["Mode"].as<String>();               // mode = "OUTPUT"
  int ledPin = device1.setIO["led"]["pin"].as<int>();       // ledPin = 2
  int ledValue = device1.setIO["led"]["value"].as<int>();   // ledValue = X            (obtém o último valor armazenado para a variável, depende das interações feitas no sistema)

  // exemplo: ligar o led sempre que iniciar o dispositivo
  
  if (mode == "OUTPUT")
  {
    device1.setIO["led"]["value"] = "1";
    device1.updatePinOutput("led");                           // atualiza o estado do pino físico associado à variável "led"
    device1.espPOST("led", setIO["led"]["value"].as<String>());    // atualiza o estado da variável na plataforma
  }
}

void loop() 
{
  device1.loop();
}
```

Com isso, se você tiver em seu dashboard componentes tipo "Botão" ou "Sinaleiro" associados à referência "led", verá seus estados se alterando conforme o dispositivo envia uma atualização utilizando 'espPOST'.

## Como construir suas próprias aplicações

### Função de callback

Para construir suas próprias aplicações ou integrar com a NodeIoT projetos já existentes, utilize a função de callback para tratar a comunicação recebida a partir das 'Referências' que você cadastrou no dispositivo criado na plataforma NodeIoT. 

Para exemplificar, observe o código básico apresentado no passo a passo do primeiro uso, esse é o código mínimo necessário para conectar um dispositivo à NodeIoT. Nele, é feita a inclusão do header 'ESP32RemoteIO.h' e declaração de um objeto 'RemoteIO' chamado 'device1'. Imediatamente abaixo está a função de callback, que por padrão, deve ser criada com o tipo 'void' e parâmetros '(String ref, String value)'. Quanto ao nome, nesse caso 'myCallback', pode ser alterado conforme desejar. 

Após essa parte, encontram-se as funções 'setup()' e 'loop()', estrutura padrão do framework Arduino. Dentro do setup deve-se fazer a inicialização do objeto RemoteIO criado, por meio da função 'device1.begin(myCallback)', onde o parâmetro necessário é a função de callback para tratar a comunicação recebida da plataforma. Por fim, no loop principal faz-se necessário chamar o loop interno do objeto RemoteIO, usando 'device1.loop()'.

Agora, observe imagem abaixo mostrando as configurações de um dispositivo criado na plataforma NodeIoT. Na sequência, serão apresentadas modificações na função de callback para tratar a comunicação recebida da plataforma, implementando um determinado comportamento como forma de exemplo.

<p align="center">
  <a href="https://nodeiot.app.br/"><img alt="Dispositivo" src="https://user-utilities.s3.us-east-1.amazonaws.com/assets/Documenta%C3%A7%C3%A3o/exemplo2.png" /></a>
</p>

### Código exemplo

```ini
#include <ESP32RemoteIO.h>

RemoteIO device1;

void myCallback(String ref, String value)
{
  if (ref == "interruptor1")
  {
    if (value == "1")
    {
      device1.setIO["luz1"]["value"] = "1";
      device1.setIO["luz2"]["value"] = "1";
      device1.setIO["motor1"]["value"] = "1";
      device1.setIO["motor2"]["value"] = "1";

      device1.updatePinOutput("luz1");
      device1.updatePinOutput("luz2");
      device1.updatePinOutput("motor1");
      device1.updatePinOutput("motor2");

      device1.espPOST("luz1", device1.setIO["luz1"]["value"].as<String>()); 
      device1.espPOST("luz2", device1.setIO["luz2"]["value"].as<String>()); 
      device1.espPOST("motor1", device1.setIO["motor1"]["value"].as<String>());
      device1.espPOST("motor2", device1.setIO["motor2"]["value"].as<String>());
    }
    else if (value == "0")
    {
      device1.setIO["luz1"]["value"] = "0";
      device1.setIO["luz2"]["value"] = "0";
      device1.setIO["motor1"]["value"] = "0";
      device1.setIO["motor2"]["value"] = "0";

      device1.updatePinOutput("luz1");
      device1.updatePinOutput("luz2");
      device1.updatePinOutput("motor1");
      device1.updatePinOutput("motor2");

      device1.espPOST("luz1", device1.setIO["luz1"]["value"].as<String>()); 
      device1.espPOST("luz2", device1.setIO["luz2"]["value"].as<String>()); 
      device1.espPOST("motor1", device1.setIO["motor1"]["value"].as<String>());
      device1.espPOST("motor2", device1.setIO["motor2"]["value"].as<String>());
    }
  }
}

void setup() 
{
  device1.begin(myCallback);
}

void loop() 
{
  device1.loop();
}
```

### Código exemplo - Explicação
Nas configurações do dispositivo mostrado existem 5 variáveis, sendo 4 delas do tipo 'OUTPUT'. Se você pesquisar um pouco, encontrará diferentes modelos de PCB baseados em esp32 contendo relés conectados diretamente aos pinos (GPIOs) do microcontrolador. O exemplo de dispositivo e código criados baseia-se em um esp32 com 4 relés.
Se você criar um dashboard com componentes do tipo "Botão On/Off" associados às variáveis do tipo 'OUTPUT', poderá ligar e desligar as saídas (conectadas às luzes e motores da piscina, no exemplo) individualmente pela plataforma.
Porém, caso deseje ligar e desligar todas essas saídas com 1 só comando, uma das maneiras de fazer isso é adicionar uma variável virtual (ex. "interruptor1") e implementar uma lógica na função de callback para aplicar o comportamento desejado.
Com um componente tipo "Botão On/Off" associado à variável "interruptor1" no dashboard, você estará enviando os comandos de ligar e desligar para o dispositivo, que receberá a referência e o valor através dos parâmetros da função de callback.
Portanto, o que esse exemplo faz basicamente é verificar se a referência que teve seu valor alterado é a "interruptor1" (que desejamos utilizar para comandar todas as saídas) e qual é o novo valor para ela (neste caso, "1" representa ligar, "0" representa desligar). 
Uma vez que essas condições tenham sido filtradas, atualiza-se o valor das variáveis que representam as saídas, dentro do objeto que armazena as configurações do dispositivo (setIO). Então, é feita a chamada da função que atualiza o estado do pino físico (GPIO) do controlador, para cada variável.
Por fim, faz-se o envio à plataforma dos novos valores dessas variáveis, para que os estados dos componentes no dashboard sejam atualizados.

## Documentação

A [documentação](https://nodeiot.com.br/docs) NodeIoT apresenta informações mais detalhadas para uso da ferramenta, onde estão disponíveis guias e exemplos para aprender a trabalhar com as entradas e saídas do dispositivo, integrando o meio físico à nuvem para construir a solução ideal para seu projeto.

