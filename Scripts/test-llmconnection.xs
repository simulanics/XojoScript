Dim host As String = "http://127.0.0.1:11434/v1/" ' Ollama local server example
Dim apiKey As String = "" ' Only required for OpenAI and Commercial API
Dim organization As String = "" ' Only required for OpenAI and Commercial API
Dim model As String = "llama3.2:latest" ' Change to the model you want to use
Dim prompt As String = "Tell me about Spartanburg, SC"
Dim maxTokens As Integer = 200
Dim temperature As Double = 0.7
Dim imagePrompt As String = "A futuristic cityscape at night"
Dim imageSize As String = "512x512"

' Set API host
LLMSetAPIHost(host)

' Set API Key and Organization (not required for Ollama)
If apiKey <> "" Then
    LLMSetConfiguration(apiKey, organization)
End If

' Create a text completion request
Dim response As String = LLMCreateCompletion(model, prompt, maxTokens, temperature)
Print "Text Completion Response: " + response

' Create an image generation request
Dim imageUrl As String = LLMCreateImage(model, imagePrompt, 1, imageSize)
Print "Generated Image URL: " + imageUrl
