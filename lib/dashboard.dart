import 'package:flutter/material.dart';
import 'complaint_report.dart';
import 'account_settings.dart';
import 'login.dart';
import 'report_history.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'dart:async';

class DashboardPage extends StatefulWidget {
  final int userId;

  const DashboardPage({
    super.key,
    required this.userId,
  });

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  double suhu = 0.0;
  double kelembapan = 0.0;
  int rpm = 0;
  bool isLoading = true;
  bool isFanOn = false;
  String userName = '';
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    fetchSensorData();
    _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
      fetchSensorData();
    });
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  Future<void> toggleFan() async {
    try {
      final response = await http.get(
        Uri.parse(
            'http://10.0.2.2/alphachill/android/submit_data.php?fan_status=${isFanOn ? 0 : 1}&id_user=1'),
      );

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        if (data['status'] == 'success') {
          await fetchSensorData();
        }
      }
    } catch (e) {
      print('Error toggling fan: $e');
    }
  }

  Future<void> fetchSensorData() async {
    try {
      final response = await http.get(Uri.parse(
          'http://10.0.2.2/alphachill/android/get_sensor_data.php?id_user=${widget.userId}'));

      if (response.statusCode == 200) {
        final data = json.decode(response.body);
        if (data['status'] == 'success') {
          if (mounted) {
            setState(() {
              try {
                suhu = double.parse(data['data']['suhu'].toString());
                kelembapan =
                    double.parse(data['data']['kelembapan'].toString());
                rpm = int.parse(data['data']['rpm'].toString());
                isFanOn = data['data']['fan_status'] == 1;
                userName = data['data']['nama'].toString();
                isLoading = false;
              } catch (e) {
                print('Error parsing data: $e');
                isLoading = false;
              }
            });
          }
        }
      }
    } catch (e) {
      print('Error fetching sensor data: $e');
      if (mounted) {
        setState(() {
          isLoading = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Image.asset(
              'images/logo.png',
              height: 40,
            ),
            const SizedBox(width: 8),
            const Text('Overview'),
          ],
        ),
        centerTitle: true,
        automaticallyImplyLeading: false,
        actions: [
          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () {
              Navigator.pushReplacement(
                context,
                MaterialPageRoute(builder: (context) => const LoginPage()),
              );
            },
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            Text(
              'Hello',
              style: TextStyle(
                fontSize: 16,
                color: Colors.grey[600],
              ),
            ),
            Text(
              userName,
              style: const TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 4),
            Text(
              DateTime.now().toString().split(' ')[0],
              style: TextStyle(color: Colors.grey[600]),
            ),
            const SizedBox(height: 24),
            Expanded(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Row(
                    children: [
                      Expanded(
                        child: _buildMonitoringCard(
                          'Temperature',
                          '${suhu.toStringAsFixed(1)}°C',
                          suhu > 30 ? 'High Level' : 'Normal Level',
                          Icons.thermostat,
                          Colors.blue,
                        ),
                      ),
                      const SizedBox(width: 16),
                      Expanded(
                        child: _buildMonitoringCard(
                          'Humidity',
                          '${kelembapan.toStringAsFixed(1)}%',
                          kelembapan > 80 ? 'High Level' : 'Normal Level',
                          Icons.water_drop,
                          Colors.red,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 16),
                  Row(
                    children: [
                      Expanded(
                        child: _buildMonitoringCard(
                          'Fan Speed',
                          '$rpm RPM',
                          rpm > 2000 ? 'High Speed' : 'Normal Speed',
                          Icons.speed,
                          Colors.orange,
                        ),
                      ),
                      const SizedBox(width: 16),
                      Expanded(
                        child: _buildMonitoringCard(
                          'Status',
                          isFanOn ? 'Active' : 'Inactive',
                          isFanOn ? 'Fan is running' : 'Fan is off',
                          Icons.power_settings_new,
                          isFanOn ? Colors.green : Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(height: 16),
            SizedBox(
              width: double.infinity,
              height: 50,
              child: ElevatedButton(
                onPressed: isLoading ? null : toggleFan,
                style: ElevatedButton.styleFrom(
                  backgroundColor: isFanOn ? Colors.red : Colors.green,
                ),
                child: Text(
                  isFanOn ? 'Turn Off Fan' : 'Turn On Fan',
                  style: const TextStyle(
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
      bottomNavigationBar: Container(
        padding: const EdgeInsets.symmetric(vertical: 10),
        decoration: BoxDecoration(
          color: Colors.white,
          boxShadow: [
            BoxShadow(
              color: Colors.grey.withOpacity(0.2),
              spreadRadius: 5,
              blurRadius: 10,
            ),
          ],
        ),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            _buildNavItem(Icons.home, 'Home', true, () {}),
            _buildNavItem(Icons.add_circle_outline, 'Report', false, () {
              Navigator.pushReplacement(
                context,
                MaterialPageRoute(
                  builder: (context) =>
                      ComplaintReportPage(userId: widget.userId),
                ),
              );
            }),
            _buildNavItem(Icons.history, 'History', false, () {
              Navigator.pushReplacement(
                context,
                MaterialPageRoute(
                  builder: (context) =>
                      ReportHistoryPage(userId: widget.userId),
                ),
              );
            }),
            _buildNavItem(Icons.person, 'Profile', false, () {
              Navigator.pushReplacement(
                context,
                MaterialPageRoute(
                  builder: (context) =>
                      AccountSettingsPage(userId: widget.userId),
                ),
              );
            }),
          ],
        ),
      ),
    );
  }

  Widget _buildMonitoringCard(
      String title, String value, String subtitle, IconData icon, Color color) {
    return Container(
      height: 180,
      decoration: BoxDecoration(
        color: color.withOpacity(0.1),
        borderRadius: BorderRadius.circular(16),
      ),
      child: isLoading
          ? const Center(child: CircularProgressIndicator())
          : Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(icon, color: color, size: 28),
                const SizedBox(height: 8),
                Text(
                  title,
                  style: TextStyle(
                    color: Colors.grey[600],
                    fontSize: 16,
                  ),
                ),
                const SizedBox(height: 8),
                Text(
                  value,
                  style: const TextStyle(
                    fontSize: 32,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  subtitle,
                  style: TextStyle(
                    color: Colors.grey[600],
                    fontSize: 14,
                  ),
                ),
              ],
            ),
    );
  }

  Widget _buildNavItem(
      IconData icon, String label, bool isSelected, VoidCallback onPressed) {
    return GestureDetector(
      onTap: onPressed,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, color: isSelected ? Colors.blue : Colors.grey, size: 24),
          Text(label,
              style: TextStyle(
                  color: isSelected ? Colors.blue : Colors.grey, fontSize: 12)),
        ],
      ),
    );
  }
}
