<?php
header('Content-Type: application/json');
include 'koneksi.php';

// Tambahkan parameter id_user dari request
$id_user = filter_input(INPUT_GET, 'id_user', FILTER_VALIDATE_INT);

if (!$id_user) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'ID User tidak valid'
    ]);
    exit;
}

// Query untuk mengambil data sensor dan user
$query = "SELECT s.id_sensor, s.suhu, s.kelembapan, s.rpm, s.fan_status, u.nama 
          FROM tb_sensor s
          JOIN user u ON s.id_user = u.id_user 
          WHERE s.id_user = ?";

$stmt = $koneksi->prepare($query);
$stmt->bind_param("i", $id_user);
$stmt->execute();
$result = $stmt->get_result();

if ($result) {
    $data = $result->fetch_assoc();
    if ($data) {
        // Tambahkan error logging
        error_log("Data found for user_id: " . $id_user);
        error_log("Data: " . json_encode($data));
        
        http_response_code(200);
        echo json_encode([
            'status' => 'success',
            'data' => [
                'suhu' => floatval($data['suhu']),
                'kelembapan' => floatval($data['kelembapan']),
                'rpm' => intval($data['rpm']),
                'fan_status' => intval($data['fan_status']),
                'nama' => $data['nama']
            ]
        ]);
    } else {
        // Log error jika data tidak ditemukan
        error_log("No data found for user_id: " . $id_user);
        
        http_response_code(404);
        echo json_encode([
            'status' => 'error',
            'message' => 'Data tidak ditemukan untuk user ini'
        ]);
    }
} else {
    // Log error database
    error_log("Database error: " . mysqli_error($koneksi));
    
    http_response_code(500);
    echo json_encode([
        'status' => 'error',
        'message' => 'Gagal mengambil data: ' . mysqli_error($koneksi)
    ]);
}

mysqli_close($koneksi);
?> 