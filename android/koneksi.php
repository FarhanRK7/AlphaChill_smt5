<?php
$koneksi = mysqli_connect("localhost", "root", "", "auto_fan");

if (!$koneksi) {
    die("Koneksi gagal: " . mysqli_connect_error());
}
?>
