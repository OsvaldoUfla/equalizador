# Equalizador JUCE

Este guia descreve o processo para executar o projeto Equalizador JUCE no Linux. Siga as instruções abaixo para configurar e construir o projeto.

## Passo a Passo

### 1. Criação do Projeto no JUCE

1. Abra o JUCE e crie um novo projeto do tipo **Basic**.
2. Nomeie o projeto como `equalizador`.
3. Adicione os seguintes módulos ao projeto:
   - `juce_dsp`
   - `juce_opengl`
   - `juce_osc`

### 2. Substituição da Pasta 'Source'

1. Substitua a pasta `Source` do projeto padrão pela pasta `Source` fornecida neste repositório. Certifique-se de que os arquivos substituídos sejam compatíveis com a estrutura e o conteúdo do projeto.

### 3. Construção do Projeto

1. Abra um terminal e navegue até a pasta `Builds/LinuxMakefile` do projeto.

   ```bash
   cd Builds/LinuxMakefile
   ```
2. Execute o comando `make` para construir o projeto.

### 4. Localização do Plugin
1. Após a conclusão da construção, entre na pasta build/myAudioProcessor.vst3/Contents/x86_64-linux.

  ```bash
  cd build/myAudioProcessor.vst3/Contents/x86_64-linux
  ```

2. Você encontrará o arquivo myAudioProcessor.so. Este arquivo pode ser renomeado, se desejado.

### 5. Adição do Plugin ao Reaper
O arquivo `equalizador.so` pode ser adicionado como um plugin no Reaper ou em outro DAW (Digital Audio Workstation) que suporte plugins VST3.

 - Abra o Reaper e vá para Options > Preferences > VST.
 - Adicione o diretório onde o arquivo myAudioProcessor.so está localizado à lista de caminhos de plugins.
 - Escaneie os plugins para que o Reaper detecte o novo plugin.

Agora você deve estar pronto para usar o Equalizador JUCE como um plugin no Reaper!
