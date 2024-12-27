<?php
header('Content-Type: application/json');
include 'koneksi.php';

$id_user = filter_input(INPUT_POST, 'id_user', FILTER_VALIDATE_INT);
$waktu = filter_input(INPUT_POST, 'waktu', FILTER_SANITIZE_STRING);
$jenis = filter_input(INPUT_POST, 'jenis', FILTER_SANITIZE_STRING);
$deskripsi = filter_input(INPUT_POST, 'deskripsi', FILTER_SANITIZE_STRING);

if (!$id_user || !$waktu || !$jenis || !$deskripsi) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'Semua field harus diisi'
    ]);
    exit;
}

try {
    // Konversi format waktu ke format MySQL datetime
    $formatted_date = date('Y-m-d H:i:s', strtotime($waktu));
    
    $query = "INSERT INTO laporan_keluhan (id_user, waktu, jenis, deskripsi) VALUES (?, ?, ?, ?)";
    $stmt = $koneksi->prepare($query);
    $stmt->bind_param("isss", $id_user, $formatted_date, $jenis, $deskripsi);
    
    if ($stmt->execute()) {
        http_response_code(200);
        echo json_encode([
            'status' => 'success',
            'message' => 'Laporan berhasil disimpan'
        ]);
    } else {
        throw new Exception("Gagal menyimpan laporan");
    }
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'status' => 'error',
        'message' => 'Gagal menyimpan laporan: ' . $e->getMessage()
    ]);
}

$koneksi->close();
?> 