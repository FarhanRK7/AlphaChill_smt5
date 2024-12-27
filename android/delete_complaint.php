<?php
header('Content-Type: application/json');
include 'koneksi.php';

$id_laporan = filter_input(INPUT_POST, 'id_laporan', FILTER_VALIDATE_INT);
$id_user = filter_input(INPUT_POST, 'id_user', FILTER_VALIDATE_INT);

if (!$id_laporan || !$id_user) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'ID tidak valid'
    ]);
    exit;
}

try {
    $query = "DELETE FROM laporan_keluhan WHERE id_laporan = ? AND id_user = ?";
    $stmt = $koneksi->prepare($query);
    $stmt->bind_param("ii", $id_laporan, $id_user);
    
    if ($stmt->execute()) {
        http_response_code(200);
        echo json_encode([
            'status' => 'success',
            'message' => 'Laporan berhasil dihapus'
        ]);
    } else {
        throw new Exception("Gagal menghapus laporan");
    }
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'status' => 'error',
        'message' => 'Gagal menghapus laporan: ' . $e->getMessage()
    ]);
}

$koneksi->close();
?> 