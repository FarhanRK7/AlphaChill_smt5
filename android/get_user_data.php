<?php
header('Content-Type: application/json');
include 'koneksi.php';

$id_user = filter_input(INPUT_GET, 'id_user', FILTER_VALIDATE_INT);

if (!$id_user) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'ID User tidak valid'
    ]);
    exit;
}

try {
    $query = "SELECT nama, email FROM user WHERE id_user = ?";
    $stmt = $koneksi->prepare($query);
    $stmt->bind_param("i", $id_user);
    $stmt->execute();
    $result = $stmt->get_result();
    
    if ($result->num_rows === 1) {
        $user = $result->fetch_assoc();
        http_response_code(200);
        echo json_encode([
            'status' => 'success',
            'data' => $user
        ]);
    } else {
        throw new Exception("User tidak ditemukan");
    }
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'status' => 'error',
        'message' => 'Gagal mengambil data: ' . $e->getMessage()
    ]);
}

$koneksi->close();
?> 