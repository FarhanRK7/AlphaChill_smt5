<?php
header('Content-Type: application/json');
include 'koneksi.php';

// Tambahkan parameter id_user
$id_user = filter_input(INPUT_GET, 'id_user', FILTER_VALIDATE_INT);

if (!$id_user) {
    http_response_code(400);
    echo json_encode([
        'status' => 'error',
        'message' => 'ID User tidak valid'
    ]);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    // Untuk update status fan
    $fan_status = filter_input(INPUT_GET, 'fan_status', FILTER_VALIDATE_INT);
    
    if ($fan_status !== null && ($fan_status === 0 || $fan_status === 1)) {
        $stmt = $koneksi->prepare("UPDATE tb_sensor SET fan_status = ? WHERE id_user = ?");
        $stmt->bind_param("ii", $fan_status, $id_user);
        
        if ($stmt->execute()) {
            http_response_code(200);
            echo json_encode(['status' => 'success', 'message' => 'Fan status updated']);
        } else {
            http_response_code(500);
            echo json_encode(['status' => 'error', 'message' => 'Failed to update fan status']);
        }
        $stmt->close();
        exit();
    }

    // Untuk update data sensor
    $temperature = filter_input(INPUT_GET, 'temperature', FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);
    $humidity = filter_input(INPUT_GET, 'humidity', FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);
    $rpm = filter_input(INPUT_GET, 'rpm', FILTER_SANITIZE_NUMBER_INT);

    if ($temperature !== null && $humidity !== null && $rpm !== null) {
        // Simpan data ke dalam database
        $stmt = $koneksi->prepare("UPDATE tb_sensor SET suhu = ?, kelembapan = ?, rpm = ? WHERE id_user = ?");
        $stmt->bind_param("ddii", $temperature, $humidity, $rpm, $id_user);

        if ($stmt->execute()) {
            http_response_code(200);
            echo json_encode(['status' => 'success', 'message' => 'Sensor data updated']);
        } else {
            http_response_code(500);
            echo json_encode(['status' => 'error', 'message' => 'Failed to update sensor data']);
        }
        $stmt->close();
    } else {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'message' => 'Invalid data']);
    }
} else {
    http_response_code(405);
    echo json_encode(['status' => 'error', 'message' => 'Invalid request method']);
}
?>
