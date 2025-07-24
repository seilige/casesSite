let isRouletteRunning = false;
let usedItems = new Set(); // Хранение уже выпавших предметов
let inventory = []; // Хранение предметов пользователя
let userData = null; // Данные пользователя

// Загрузка данных пользователя при загрузке страницы
document.addEventListener('DOMContentLoaded', function() {
    loadUserData();
});

function loadUserData() {
    fetch('/user_data')
        .then(response => response.json())
        .then(data => {
            userData = data;
            updateUserInterface();
        })
        .catch(error => {
            console.error('Error loading user data:', error);
            // Если ошибка загрузки данных пользователя, перенаправляем на главную
            window.location.href = '/';
        });
}

function updateUserInterface() {
    if (!userData) return;
    
    // Обновляем аватар и имя пользователя
    const avatar = document.querySelector('.avatar');
    const profileLink = document.querySelector('header a');
    
    if (avatar) {
        avatar.src = userData.avatar;
    }
    
    if (profileLink) {
        profileLink.textContent = userData.username;
    }
    
    // Загружаем инвентарь пользователя
    inventory = userData.items || [];
    updateInventoryUI();
}

function openCase(caseId) {
    fetch(`/open_case?case_id=${caseId}`)
        .then(response => response.json())
        .then(data => {
            const roulette = document.getElementById('roulette');
            const caseItems = document.getElementById('case-items');
            const modal = document.getElementById('case-modal');

            // Очистить предыдущие данные
            roulette.innerHTML = '';
            caseItems.innerHTML = '';
            usedItems.clear(); // Сбросить использованные предметы

            // Генерация случайного порядка элементов
            const shuffledItems = [...data.items, ...data.items, ...data.items]
                .sort(() => Math.random() - 0.5);

            // Добавить изображения в рулетку
            shuffledItems.forEach(item => {
                const img = document.createElement('img');
                img.src = item;
                roulette.appendChild(img);
            });

            // Добавить изображения в список
            data.items.forEach(item => {
                const img = document.createElement('img');
                img.src = item;
                caseItems.appendChild(img);
            });

            // Показать модальное окно
            modal.classList.remove('hidden');

            // Закрытие модального окна при клике вне содержимого
            modal.addEventListener('click', (event) => {
                if (event.target === modal) {
                    closeModal();
                }
            });
        })
        .catch(error => {
            console.error('Error:', error);
        });
}

function startRoulette() {
    if (isRouletteRunning) return; // Предотвращение повторного запуска
    isRouletteRunning = true;

    const roulette = document.getElementById('roulette');
    const items = Array.from(roulette.children); // Получить текущие элементы рулетки

    // Генерация новой случайной ленты
    const shuffledItems = items.map(item => item.cloneNode(true)) // Клонируем текущие элементы
        .sort(() => Math.random() - 0.5);

    // Очистить текущую ленту и добавить новую
    roulette.innerHTML = ''; // Удаляем старую ленту
    shuffledItems.forEach(item => {
        roulette.appendChild(item);
    });

    // Сброс анимации
    roulette.style.transition = 'none';
    roulette.style.transform = 'translateX(0)';

    // Запуск анимации
    setTimeout(() => {
        const totalScroll = roulette.scrollWidth / 3; // Прокрутка одной трети ленты
        roulette.style.transition = 'transform 4s linear'; // Фиксированное время прокрутки
        roulette.style.transform = `translateX(-${totalScroll}px)`;

        // Сброс состояния после завершения
        setTimeout(() => {
            isRouletteRunning = false;

            // Показать случайный выбранный предмет
            let randomIndex;
            let selectedItem;

            // Исключить уже выпавшие предметы
            do {
                randomIndex = Math.floor(Math.random() * items.length);
                selectedItem = items[randomIndex];
            } while (usedItems.has(selectedItem.src));

            usedItems.add(selectedItem.src); // Добавить предмет в список использованных
            animateFullscreenItem(selectedItem.src);

            // Добавить предмет в инвентарь
            addToInventory(selectedItem.src);
        }, 4000);
    }, 0);
}

function addToInventory(item) {
    inventory.push(item);
    updateInventoryUI();
}

function updateInventoryUI() {
    const inventoryContainer = document.getElementById('inventory-items');
    if (!inventoryContainer) return;
    
    inventoryContainer.innerHTML = ''; // Очистить текущий инвентарь

    inventory.forEach(item => {
        const img = document.createElement('img');
        img.src = item;
        inventoryContainer.appendChild(img);
    });
}

function toggleInventory() {
    const inventoryModal = document.getElementById('inventory-modal');
    if (inventoryModal) {
        inventoryModal.classList.toggle('hidden');
    }
}

function closeModal() {
    const modal = document.getElementById('case-modal');
    modal.classList.add('hidden');
    const roulette = document.getElementById('roulette');
    roulette.style.animation = 'none'; // Остановить анимацию
    roulette.style.transform = ''; // Сбросить позицию рулетки
}

function animateFullscreenItem(src) {
    const fullscreen = document.getElementById('fullscreen-item');
    const fullscreenImage = document.getElementById('fullscreen-image');
    fullscreenImage.src = src;
    fullscreen.classList.remove('hidden');

    // Добавить анимацию появления
    fullscreen.classList.add('fade-in');

    // Удалить анимацию через 3 секунды
    setTimeout(() => {
        fullscreen.classList.remove('fade-in');
        fullscreen.classList.add('fade-out');

        // Закрыть fullscreen через 1 секунду после завершения анимации
        setTimeout(() => {
            fullscreen.classList.add('hidden');
            fullscreen.classList.remove('fade-out');
        }, 1000);
    }, 3000);
}

function logout() {
    fetch('/logout', {
        method: 'GET'
    })
    .then(() => {
        window.location.href = '/';
    })
    .catch(error => {
        console.error('Error during logout:', error);
        window.location.href = '/';
    });
}
