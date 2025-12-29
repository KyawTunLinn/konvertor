document.addEventListener('DOMContentLoaded', () => {
    const uploadArea = document.getElementById('uploadArea');
    const fileInput = document.getElementById('fileInput');
    const fileInfo = document.getElementById('fileInfo');
    const convertBtn = document.getElementById('convertBtn');
    const statsDisplay = document.getElementById('statsDisplay');

    // Function to fetch stats
    function fetchStats() {
        fetch('/api/stats')
            .then(response => response.json())
            .then(data => {
                if (data.total_conversions !== undefined) {
                    statsDisplay.textContent = `Total Conversions: ${data.total_conversions}`;
                }
            })
            .catch(err => console.error('Error fetching stats:', err));
    }

    // Fetch immediately and then every 2 seconds for realtime feel
    fetchStats();
    setInterval(fetchStats, 2000);

    const formatSelect = document.getElementById('formatSelect');
    const qualitySelect = document.getElementById('qualitySelect');
    const progressContainer = document.getElementById('progressContainer');
    const progressBar = document.getElementById('progressBar');
    const statusText = document.getElementById('statusText');
    const progressPercent = document.getElementById('progressPercent');
    const resultArea = document.getElementById('resultArea');
    const downloadLink = document.getElementById('downloadLink');
    const resetBtn = document.getElementById('resetBtn');
    const controls = document.querySelector('.controls');
    const batchFileList = document.getElementById('batchFileList');

    let selectedFiles = [];

    // Drag and Drop events
    ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
        uploadArea.addEventListener(eventName, preventDefaults, false);
    });

    function preventDefaults(e) {
        e.preventDefault();
        e.stopPropagation();
    }

    ['dragenter', 'dragover'].forEach(eventName => {
        uploadArea.addEventListener(eventName, highlight, false);
    });

    ['dragleave', 'drop'].forEach(eventName => {
        uploadArea.addEventListener(eventName, unhighlight, false);
    });

    function highlight(e) {
        uploadArea.classList.add('drag-over');
    }

    function unhighlight(e) {
        uploadArea.classList.remove('drag-over');
    }

    uploadArea.addEventListener('drop', handleDrop, false);
    uploadArea.addEventListener('click', () => fileInput.click());

    function handleDrop(e) {
        const dt = e.dataTransfer;
        const files = Array.from(dt.files).filter(f => f.type.startsWith('video/'));
        handleFiles(files);
    }

    fileInput.addEventListener('change', (e) => {
        const files = Array.from(e.target.files).filter(f => f.type.startsWith('video/'));
        handleFiles(files);
    });

    function handleFiles(files) {
        if (files.length === 0) {
            // Optional: nice toast instead of alert? Keeping alert for simplicity for now.
            alert('Please select video files.');
            return;
        }

        selectedFiles = files;
        fileInfo.textContent = `Selected ${files.length} file${files.length === 1 ? '' : 's'}`;

        // ALWAYS use batch list, even for single file
        displayBatchList(files);
        batchFileList.style.display = 'block';

        convertBtn.disabled = false;
        progressContainer.style.display = 'none';
        resultArea.style.display = 'none';
    }

    function displayBatchList(files) {
        batchFileList.innerHTML = '';
        batchFileList.style.display = 'block';

        files.forEach((file, index) => {
            const fileItem = document.createElement('div');
            fileItem.className = 'batch-file-item';
            fileItem.innerHTML = `
                <div class="batch-file-info">
                    <span class="file-name">${index + 1}. ${file.name}</span>
                    <span class="file-size">${(file.size / (1024 * 1024)).toFixed(2)} MB</span>
                    <span class="file-status" id="status-${index}">Pending</span>
                </div>
                <!-- Progress container always visible but empty -->
                <div class="batch-progress-container" id="progress-container-${index}">
                    <div class="batch-progress-bar">
                        <div class="batch-progress-fill" id="progress-fill-${index}" style="width: 0%"></div>
                    </div>
                    <span class="batch-progress-percent" id="progress-percent-${index}">0%</span>
                </div>
            `;
            batchFileList.appendChild(fileItem);
        });
    }

    convertBtn.addEventListener('click', uploadAndConvert);

    resetBtn.addEventListener('click', () => {
        // Reset everything
        selectedFiles = [];
        fileInput.value = '';
        fileInfo.textContent = 'or click to browse files';
        convertBtn.disabled = true;
        progressContainer.style.display = 'none';
        resultArea.style.display = 'none';
        uploadArea.style.display = 'block';
        controls.style.display = 'grid'; // Changed from flex to grid to match CSS
        batchFileList.style.display = 'none'; // Hide batch list on reset
        batchFileList.innerHTML = '';
        progressBar.style.width = '0%';
        progressPercent.textContent = '0%';
        statusText.textContent = 'Ready to convert';

        // Remove drag-over class just in case
        uploadArea.classList.remove('drag-over');
    });

    async function uploadAndConvert() {
        if (selectedFiles.length === 0) return;

        // Hide upload area and controls
        controls.style.display = 'none';
        uploadArea.style.display = 'none';
        resultArea.style.display = 'none';

        const downloadUrls = [];

        // Process files sequentially
        for (let i = 0; i < selectedFiles.length; i++) {
            const file = selectedFiles[i];
            const statusSpan = document.getElementById(`status-${i}`);

            if (statusSpan) {
                statusSpan.textContent = 'Converting...';
                statusSpan.style.color = 'var(--accent-color)';
            }

            try {
                const downloadUrl = await convertSingleFile(file, i);
                downloadUrls.push(downloadUrl);

                if (statusSpan) {
                    statusSpan.innerHTML = `
                        <a href="${downloadUrl}" download class="batch-download-btn">
                            <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path>
                                <polyline points="7 10 12 15 17 10"></polyline>
                                <line x1="12" y1="15" x2="12" y2="3"></line>
                            </svg>
                            Download
                        </a>`;
                }
            } catch (error) {
                if (statusSpan) {
                    statusSpan.textContent = '✗ Failed';
                    statusSpan.style.color = 'var(--error-color)';
                }
            }
        }

        // Show success modal instead of alert
        showSuccessModal(
            'Batch Conversion Complete!',
            `Successfully processed ${downloadUrls.length} file(s).`
        );

        // Show the main result area too so they can reset
        resultArea.style.display = 'block';

        // Update logic for big download button
        if (downloadUrls.length === 1) {
            downloadLink.href = downloadUrls[0];
            downloadLink.innerHTML = `
                <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                    <path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z" />
                </svg>
                Download Audio`;
            downloadLink.style.display = 'inline-flex';
        } else if (downloadUrls.length > 1) {
            // New Zip Logic
            const files = downloadUrls.map(url => url.split('/').pop()); // Extract filenames

            fetch('/api/zip', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ files: files })
            })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        downloadLink.href = data.download_url;
                        downloadLink.innerHTML = `
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                            <path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z" />
                        </svg>
                        Download All (ZIP)`;
                        downloadLink.style.display = 'inline-flex';
                    } else {
                        console.error('Zip creation failed:', data.error);
                    }
                })
                .catch(err => {
                    console.error('Zip request error:', err);
                });
        } else {
            downloadLink.style.display = 'none';
        }
    }

    // Custom modal functions
    function showSuccessModal(title, message) {
        const modal = document.getElementById('successModal');
        const modalTitle = document.getElementById('modalTitle');
        const modalMessage = document.getElementById('modalMessage');
        const closeBtn = document.getElementById('modalCloseBtn');

        modalTitle.textContent = title;
        modalMessage.textContent = message;
        modal.style.display = 'flex';

        // Close on button click
        closeBtn.onclick = () => {
            modal.style.display = 'none';
        };

        // Close on overlay click
        modal.onclick = (e) => {
            if (e.target === modal) {
                modal.style.display = 'none';
            }
        };

        // Close on Escape key
        const escapeHandler = (e) => {
            if (e.key === 'Escape') {
                modal.style.display = 'none';
                document.removeEventListener('keydown', escapeHandler);
            }
        };
        document.addEventListener('keydown', escapeHandler);
    }

    function convertSingleFile(file, index) {
        return new Promise((resolve, reject) => {
            const formData = new FormData();
            formData.append('file', file);
            formData.append('format', formatSelect.value);
            formData.append('quality', qualitySelect.value);

            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/api/convert', true);

            const progressFill = document.getElementById(`progress-fill-${index}`);
            const progressPercent = document.getElementById(`progress-percent-${index}`);
            const statusSpan = document.getElementById(`status-${index}`);

            let conversionInterval = null;

            // Track upload progress
            xhr.upload.onprogress = function (e) {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    if (progressFill && progressPercent) {
                        progressFill.style.width = percent + '%';
                        progressPercent.textContent = percent + '%';
                    }
                    if (statusSpan) {
                        statusSpan.textContent = 'Uploading...';
                        statusSpan.style.color = 'var(--accent-color)';
                    }
                }
            };

            // Upload complete, start simulated conversion progress
            xhr.upload.onload = function () {
                if (statusSpan) {
                    statusSpan.textContent = 'Converting...';
                }

                // Simulate conversion progress
                let conversionProgress = 30;
                conversionInterval = setInterval(() => {
                    if (xhr.readyState === 4) {
                        clearInterval(conversionInterval);
                        return;
                    }
                    if (conversionProgress < 95) {
                        conversionProgress += (Math.random() * 3);
                        const pct = Math.round(conversionProgress);
                        if (progressFill && progressPercent) {
                            progressFill.style.width = pct + '%';
                            progressPercent.textContent = pct + '%';
                        }
                    }
                }, 300);
            };

            xhr.onload = function () {
                // Clear conversion interval
                if (conversionInterval) {
                    clearInterval(conversionInterval);
                }

                // Set to 100%
                if (progressFill && progressPercent) {
                    progressFill.style.width = '100%';
                    progressPercent.textContent = '100%';
                }

                if (xhr.status === 200) {
                    const response = JSON.parse(xhr.responseText);
                    if (response.status === 'success') {
                        if (statusSpan) {
                            statusSpan.innerHTML = `
                                <a href="${response.download_url}" download class="batch-download-btn">
                                    <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                        <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path>
                                        <polyline points="7 10 12 15 17 10"></polyline>
                                        <line x1="12" y1="15" x2="12" y2="3"></line>
                                    </svg>
                                    Download
                                </a>`;
                        }
                        resolve(response.download_url);
                    } else {
                        if (statusSpan) {
                            statusSpan.textContent = '✗ Failed';
                            statusSpan.style.color = 'var(--error-color)';
                        }
                        reject(new Error(response.error || 'Conversion failed'));
                    }
                } else {
                    if (statusSpan) {
                        statusSpan.textContent = '✗ Error';
                        statusSpan.style.color = 'var(--error-color)';
                    }
                    reject(new Error('HTTP error: ' + xhr.status));
                }
            };

            xhr.onerror = function () {
                if (conversionInterval) {
                    clearInterval(conversionInterval);
                }
                if (statusSpan) {
                    statusSpan.textContent = '✗ Network Error';
                    statusSpan.style.color = 'var(--error-color)';
                }
                reject(new Error('Network error'));
            };

            xhr.send(formData);
        });
    }

    function finishConversion(url, format) {
        progressContainer.style.display = 'none';
        resultArea.style.display = 'block';
        downloadLink.href = url;
    }

    // Global drag prevention
    window.addEventListener('dragover', function (e) {
        e.preventDefault();
        if (progressContainer.style.display === 'block') {
            e.dataTransfer.dropEffect = 'none';
        }
    }, false);

    window.addEventListener('drop', function (e) {
        e.preventDefault();
        // If we are converting (progress container is visible), ignore drops completely
        if (progressContainer.style.display === 'block') {
            e.stopPropagation();
            return false;
        }
    }, false);
});
