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
    $query = "SELECT * FROM laporan_keluhan WHERE id_user = ? ORDER BY waktu DESC";
    $stmt = $koneksi->prepare($query);
    $stmt->bind_param("i", $id_user);
    $stmt->execute();
    $result = $stmt->get_result();
    
    $complaints = [];
    while ($row = $result->fetch_assoc()) {
        $complaints[] = [
            'id' => $row['id_laporan'],
            'date' => $row['waktu'],
            'type' => $row['jenis'],
            'description' => $row['deskripsi']
        ];
    }
    
    http_response_code(200);
    echo json_encode([
        'status' => 'success',
        'data' => $complaints
    ]);
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'status' => 'error',
        'message' => 'Gagal mengambil data: ' . $e->getMessage()
    ]);
}

$koneksi->close();
?> 