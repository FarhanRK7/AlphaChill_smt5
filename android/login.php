<?php
header('Content-Type: application/json');
include 'koneksi.php';

// Terima data dari request POST
$username = filter_input(INPUT_POST, 'username', FILTER_SANITIZE_STRING);
$password = filter_input(INPUT_POST, 'password', FILTER_SANITIZE_STRING);

// Validasi input
if (!$username || !$password) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'Username dan password harus diisi'
    ]);
    exit;
}

try {
    // Query langsung membandingkan password dalam plain text
    $query = "SELECT id_user, nama, username FROM user WHERE username = ? AND password = ?";
    $stmt = $koneksi->prepare($query);
    $stmt->bind_param("ss", $username, $password);
    $stmt->execute();
    $result = $stmt->get_result();
    
    if ($result->num_rows === 1) {
        $user = $result->fetch_assoc();
        
        // Login berhasil
        http_response_code(200);
        echo json_encode([
            'status' => 'success',
            'message' => 'Login berhasil',
            'data' => [
                'id_user' => $user['id_user'],
                'nama' => $user['nama'],
                'username' => $user['username']
            ]
        ]);
    } else {
        // Login gagal
        http_response_code(401);
        echo json_encode([
            'status' => 'error',
            'message' => 'Username atau password salah'
        ]);
    }
    
} catch (Exception $e) {
    error_log("Login error: " . $e->getMessage());
    http_response_code(500);
    echo json_encode([
        'status' => 'error',
        'message' => 'Terjadi kesalahan pada server'
    ]);
}

$koneksi->close();
?> 