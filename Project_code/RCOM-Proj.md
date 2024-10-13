## Objetivos
- Desenvolver um Data Linker Layer Protocol que implementa funcionalidades para transferir para o receiver e sender que permite enviar um ficheiro
- Desenvolver um aplicação simples de transferencia de dados entre o sender e receiver que usa a API do Data Linker Layer Protocol

## Data Linker Layer Protocol
### Funcionalidades a implementar:

**Framing**

- Colocar os packets do níveis superiores em frame. Composto por header | body | trailer. O body contem o packet. Este frame são os I (information) frames.
- Para sincronizar os frames podem ser usadas flagss
- Para garantir que informação no body não é confundido com uma flag pode ser usado stuffing
- O tamanho do frame pode ser deduzido, contando o nº de bytes entre as flags.

-------------

- **Connection establishment and termination**

- Frames que apenas contêm informação de controlo, não têm body. Chamados de Supervision Frames.

-------------
- **Frame numbering**

- No header, incluir a numeração para ser possível saber a ordem e destinguir repetidos.

-------------
- **Acknowledgement**

- Enviar um ack sempre que um frame é recebido na ordem correta e sem erros

-------------
- **Error control**

- Usar timer para reenciar frames em que o ack respetivo não foi recebido
- Usar ack negativos para pedir o reenvio de frames
- Verificar a ocorrencia de duplicados

-------------
- **Flow control**

-------------

### Especificação do Protocolo
- Tranmissão organizada em packets (Information (I), Supervision (S) and Unnumbered (U))
- Frames têm um header comum, sendo que apenas os I têm um field para transportar packets
- Frames são limitados por Flag e é usado bit stuffing para que não seja confundida uma Flag com a informação transportada
- Frames estão protegidos contra erros
- É para usar a variação **Stop and Wait com janela e operações modulo 2**


## Diferentes Frames
![](assets/SFrames.png)
![](assets/IFrame.png)

## Funcionamento:

- Headers errados, são ignorados. Ou seja, não são enviados ack

- Se o I frame não tiver erros:
    - Se for novo é aceite e enviado um ack (RR)
    - Se for duplicado, não é aceite, mas o RR tem de ser enviado

- Se o I frame tiver erros apenas no data field:
    - Se for novo, é enviado um ack com Control REJ, pois evita esperar pelo time-out
    - Se for repetido, é enviado o ack com RR.

- Ou seja, se o header tiver erros, não é enviado ack, se o erro estiver apenas no data field é pedido de novo, mas evitando o time-out

- Tem de existir um limite de retransmissões
- Se for encontrada a flag no data field, 0x7E passa a 0x7D5E
- Se for encontrado o escape byte, 0x7D passa a 0x7D5D

- Retransmissões de I frames:
    - Se acabar o time-out
    - Se receber um ack REJ

- I frames fazem xor de todos os bytes antes e depois do bit stuffing para um campo de verificação de erros (BCC)

- Para terminar a conexão, o Sender envia DISC, o Receiver responde com um DISC e o Sender envia um UA a confirmar que recebeu o DISC, isto pois (Onde colocar? Dentro do llwrite e llread, isto é, as maquinas de estado têm a lógica para verificar se é uma flag? O mais lógico é no llclose, mas a função não esta a receber o LinkLayer):
    - O último a a sair da conexão tem de ser o Receiver, para evitar o seguinte caso. Se o sender enviar o DISC e o receiver enviar o DISC e desconectar, caso o DISC do receiver se perder, o sender voltaria a enviar o DISC passado o time-out, ficando assim num ciclo infinito

TODO

Organizar states machine no llopen