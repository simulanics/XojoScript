Dim host As String = "http://127.0.0.1:11434/v1/" ' Ollama local server example
Dim apiKey As String = "" ' Only required for OpenAI and Commercial API
Dim organization As String = "" ' Only required for OpenAI and Commercial API
Dim model As String = "llama3.2:latest" ' Change to the model you want to use
Dim prompt As String = "Write an essay about Spartanburg, SC in markdown format"
Dim maxTokens As Integer = 1024
Dim temperature As Double = 0.7
Dim imagePrompt As String = "A futuristic cityscape at night"
Dim imageSize As String = "512x512"

' Set API host URL
LLMSetAPIHost(host)

' Set API Key and Organization (not required for Ollama)
If apiKey <> "" Then
    LLMSetConfiguration(apiKey, organization)
End If

' Create a text completion request
Dim response As String = LLMCreateCompletion(model, prompt, maxTokens, temperature)
Print "Text Completion Response: "
PrintColorMarkdown(response)

' Create an image generation request - OpenAI and image models using OpenAI API only.
' Dim imageUrl As String = LLMCreateImage(model, imagePrompt, 1, imageSize)
' Print "Generated Image URL: " + imageUrl

Dim response2 As String = LLMCreateCompletion(model, "Create an outline for a fictional novel for young adults ages 18-22.", maxTokens, temperature)
Print "Novel Outline: "
PrintColorMarkdown(response2)
return 0