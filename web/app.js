// Chat application for Melvin (HTTP API version)

const messagesContainer = document.getElementById('messages');
const messageInput = document.getElementById('messageInput');
const sendButton = document.getElementById('sendButton');
const statusSpan = document.getElementById('status');
const errorRateSpan = document.getElementById('errorRate');

let isLoading = false;

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

// Send message to Melvin via HTTP API
async function sendMessage() {
    const message = messageInput.value.trim();
    
    if (!message || isLoading) {
        return;
    }
    
    // Add user message to chat
    addMessage(message, true);
    messageInput.value = '';
    
    // Disable input
    isLoading = true;
    messageInput.disabled = true;
    sendButton.disabled = true;
    updateStatus('Sending...');
    
    try {
        // Send to API
        const response = await fetch('/api/chat', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ message: message })
        });
        
        if (!response.ok) {
            const errorData = await response.json().catch(() => ({ error: 'Unknown error' }));
            throw new Error(errorData.error || `HTTP ${response.status}`);
        }
        
        const data = await response.json();
        
        // Add Melvin's response
        if (data.response) {
            addMessage(data.response, false);
        } else {
            addMessage('(No response)', false);
        }
        
        // Update error rate
        if (data.error_rate !== undefined) {
            updateErrorRate(data.error_rate);
        }
        
        updateStatus('Ready');
        
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

// Check server status on load
async function checkStatus() {
    try {
        const response = await fetch('/api/status');
        if (response.ok) {
            const data = await response.json();
            updateStatus('Connected');
            if (data.error_rate !== undefined) {
                updateErrorRate(data.error_rate);
            }
        } else {
            updateStatus('Server error', true);
        }
    } catch (error) {
        updateStatus('Cannot connect to server', true);
    }
}

// Initialize
messageInput.focus();
checkStatus();

// Auto-focus input when clicking anywhere in chat
messagesContainer.addEventListener('click', () => {
    messageInput.focus();
});
