// Chat application for Melvin (WebAssembly version - runs on user's computer)

const messagesContainer = document.getElementById('messages');
const messageInput = document.getElementById('messageInput');
const sendButton = document.getElementById('sendButton');
const statusSpan = document.getElementById('status');
const errorRateSpan = document.getElementById('errorRate');

let isLoading = false;
let melvinModule = null;
let melvinProcess = null;
let melvinGetError = null;
let melvinInit = null;
let melvinFree = null;

// Add message to chat
function addMessage(text, isUser) {
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${isUser ? 'user-message' : 'melvin-message'}`;
    
    const contentDiv = document.createElement('div');
    contentDiv.className = 'message-content';
    
    const headerDiv = document.createElement('div');
    headerDiv.className = 'message-header';
    headerDiv.textContent = isUser ? 'You' : 'Melvin';
    
    const textDiv = document.createElement('div');
    textDiv.className = 'message-text';
    textDiv.textContent = text;
    
    contentDiv.appendChild(headerDiv);
    contentDiv.appendChild(textDiv);
    messageDiv.appendChild(contentDiv);
    
    messagesContainer.appendChild(messageDiv);
    
    // Scroll to bottom
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
}

// Update status
function updateStatus(text, isError = false) {
    statusSpan.textContent = text;
    if (isError) {
        statusSpan.style.color = '#e74c3c';
    } else {
        statusSpan.style.color = '#666';
    }
}

// Update error rate
function updateErrorRate(errorRate) {
    if (errorRate !== undefined && errorRate !== null) {
        errorRateSpan.textContent = `Error Rate: ${errorRate.toFixed(3)}`;
    } else {
        errorRateSpan.textContent = '';
    }
}

// Initialize WebAssembly module
async function initMelvin() {
    try {
        updateStatus('Loading Melvin...');
        
        // Load the WebAssembly module
        melvinModule = await Module();
        
        // Get function wrappers
        melvinInit = melvinModule.cwrap('melvin_init', 'number', []);
        melvinProcess = melvinModule.cwrap('melvin_process_message', 'string', ['string']);
        melvinGetError = melvinModule.cwrap('melvin_get_error', 'number', []);
        melvinFree = melvinModule.cwrap('melvin_free_string', null, ['string']);
        
        // Initialize Melvin
        const result = melvinInit();
        if (result === 0) {
            throw new Error('Failed to initialize Melvin');
        }
        
        updateStatus('Ready (Running on your computer)');
        updateErrorRate(melvinGetError());
        
        console.log('Melvin WebAssembly loaded successfully - running locally!');
        
    } catch (error) {
        console.error('Failed to load Melvin:', error);
        updateStatus('Failed to load Melvin', true);
        addMessage('Error: Failed to initialize Melvin. Please refresh the page.', false);
    }
}

// Send message to Melvin
function sendMessage() {
    const message = messageInput.value.trim();
    
    if (!message || isLoading || !melvinProcess) {
        return;
    }
    
    // Add user message to chat
    addMessage(message, true);
    messageInput.value = '';
    
    // Disable input
    isLoading = true;
    messageInput.disabled = true;
    sendButton.disabled = true;
    updateStatus('Processing...');
    
    try {
        // Process message through Melvin (runs on user's computer!)
        const response = melvinProcess(message);
        
        // Add Melvin's response
        if (response) {
            addMessage(response, false);
            // Free the string returned by WebAssembly
            if (melvinFree) {
                melvinFree(response);
            }
        } else {
            addMessage('(No response)', false);
        }
        
        // Update error rate
        if (melvinGetError) {
            updateErrorRate(melvinGetError());
        }
        
        updateStatus('Ready (Running on your computer)');
        
    } catch (error) {
        console.error('Error:', error);
        addMessage(`Error: ${error.message}`, false);
        updateStatus(`Error: ${error.message}`, true);
    } finally {
        // Re-enable input
        isLoading = false;
        messageInput.disabled = false;
        sendButton.disabled = false;
        messageInput.focus();
    }
}

// Handle Enter key
messageInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        sendMessage();
    }
});

// Initialize on page load
window.addEventListener('load', () => {
    messageInput.focus();
    initMelvin();
});

// Auto-focus input when clicking anywhere in chat
messagesContainer.addEventListener('click', () => {
    messageInput.focus();
});
