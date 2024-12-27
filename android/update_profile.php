<?php
header('Content-Type: application/json');
include 'koneksi.php';

$id_user = filter_input(INPUT_POST, 'id_user', FILTER_VALIDATE_INT);
$nama = filter_input(INPUT_POST, 'nama', FILTER_SANITIZE_STRING);
$email = filter_input(INPUT_POST, 'email', FILTER_SANITIZE_EMAIL);
$password = filter_input(INPUT_POST, 'password', FILTER_SANITIZE_STRING);

if (!$id_user || !$nama || !$email) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'Data tidak lengkap'
    ]);
    exit;
}

try {
    if ($password) {
        $query = "UPDATE user SET nama = ?, email = ?, password = ? WHERE id_user = ?";
        $stmt = $koneksi->prepare($query);
        $stmt->bind_param("sssi", $nama, $email, $password, $id_user);
    } else {
        $query = "UPDATE user SET nama = ?, email = ? WHERE id_user = ?";
        $stmt = $koneksi->prepare($query);
        $stmt->bind_param("ssi", $nama, $email, $id_user);
    }
    
    if ($stmt->execute()) {
        http_response_code(200);
        echo json_encode([
            'status' => 'success',
            'message' => 'Profil berhasil diupdate'
        ]);
    } else {
        throw new Exception("Gagal mengupdate profil");
    }
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'status' => 'error',
        'message' => 'Gagal mengupdate profil: ' . $e->getMessage()
    ]);
}

$koneksi->close();
?> 