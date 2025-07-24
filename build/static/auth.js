function switchTab(tabName) {
    const buttons = document.querySelectorAll('.tab-button');
    const loginForm = document.getElementById('login-form');
    const registerForm = document.getElementById('register-form');
    
    buttons.forEach(btn => btn.classList.remove('active'));
    
    if (tabName === 'login') {
        buttons[0].classList.add('active');
        loginForm.classList.add('active');
        registerForm.classList.remove('active');
    } else if (tabName === 'register') {
        buttons[1].classList.add('active');
        registerForm.classList.add('active');
        loginForm.classList.remove('active');
    }
    
    // Очищаем формы при переключении
    loginForm.reset();
    registerForm.reset();
    updateAvatarPreview();
}

function updateAvatarPreview() {
    const select = document.getElementById('register-avatar');
    const preview = document.getElementById('avatar-preview');
    preview.src = select.value;
}

function showMessage(text, type = 'info') {
    const messageDiv = document.getElementById('message');
    messageDiv.textContent = text;
    messageDiv.className = 'message ' + type;
    messageDiv.style.display = 'block';
}

function hideMessage() {
    const messageDiv = document.getElementById('message');
    messageDiv.style.display = 'none';
}
function handleLogin(event) {
    event.preventDefault();

    const formData = new FormData(event.target);
    const username = formData.get('username');
    const password = formData.get('password');

    if (!username || !password) {
        showMessage('Пожалуйста, заполните все поля', 'error');
        return;
    }

    fetch('/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}`
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showMessage(data.message, 'success');
            setTimeout(() => {
                window.location.href = '/cases';
            }, 1500); // Увеличьте задержку, если нужно
        } else {
            showMessage(data.message, 'error');
        }
    })
    .catch(error => {
        console.error('Error:', error);
        showMessage('Произошла ошибка при входе', 'error');
    });
}

function handleRegister(event) {
    event.preventDefault();

    const formData = new FormData(event.target);
    const username = formData.get('username');
    const password = formData.get('password');
    const avatar = formData.get('avatar');

    if (!username || !password) {
        showMessage('Пожалуйста, заполните все поля', 'error');
        return;
    }

    fetch('/register', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `username=${encodeURIComponent(username)}&password=${encodeURIComponent(password)}&avatar=${encodeURIComponent(avatar)}`
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showMessage(data.message, 'success');
            setTimeout(() => {
                window.location.href = '/cases';
            }, 1500); // Увеличьте задержку, если нужно
        } else {
            showMessage(data.message, 'error');
        }
    })
    .catch(error => {
        console.error('Error:', error);
        showMessage('Произошла ошибка при регистрации', 'error');
    });
}

document.addEventListener('DOMContentLoaded', function() {
    updateAvatarPreview();
    
    // Получаем формы
    const loginForm = document.getElementById('login-form');
    const registerForm = document.getElementById('register-form');
    
    // Привязываем обработчики к формам
    if (loginForm) {
        loginForm.addEventListener('submit', handleLogin);
    }
    
    if (registerForm) {
        registerForm.addEventListener('submit', handleRegister);
    }
    
    // Обработчики для кнопок переключения вкладок
    const loginTab = document.querySelector('.tab-button:first-child');
    const registerTab = document.querySelector('.tab-button:last-child');
    
    if (loginTab) {
        loginTab.addEventListener('click', function() {
            switchTab('login');
            hideMessage(); // Скрываем сообщения при переключении
        });
    }
    
    if (registerTab) {
        registerTab.addEventListener('click', function() {
            switchTab('register');
            hideMessage(); // Скрываем сообщения при переключении
        });
    }
});