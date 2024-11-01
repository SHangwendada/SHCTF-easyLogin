package com.swdd.easylogin;

import androidx.appcompat.app.AppCompatActivity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.StrictMode;
import android.text.InputType;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;
import com.swdd.easylogin.databinding.ActivityMainBinding;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("easylogin");
    }

    private ActivityMainBinding binding;
    private String serverAddress;  // 保存服务器地址
    private int responseCode;
    private int responseCode2;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // 允许网络请求在主线程中运行（仅用于测试）
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
        StrictMode.setThreadPolicy(policy);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // 弹出服务器地址输入框，直到连接成功
        promptForServerAddress();

        // 设置Login按钮的点击事件
        binding.loginButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                handleLogin();
            }
        });
    }

    private void handleLogin() {
        String username = binding.username.getText().toString().trim();
        String password = binding.password.getText().toString().trim();
        String passticket = getPassticket();  // 在实际场景中可通过输入框或其他方式获取

        if (username.isEmpty() || password.isEmpty()) {
            Toast.makeText(this, "Please enter both username and password.", Toast.LENGTH_SHORT).show();
            return;
        }

        String requestUrl = serverAddress + "?username=" + username + "&password=" + password + "&passticket=" + passticket;

        try {
            String response = sendRequest(requestUrl);
            if (response != null && response.startsWith("SHCTF")) {
                showAlert("Login Successful", response);
            }else if(responseCode2 == 203){
                Toast.makeText(this, "检测到新设备登录，请在旧设备上验证。", Toast.LENGTH_SHORT).show();
            }else {
                Toast.makeText(this, "Login failed. Check your credentials.", Toast.LENGTH_SHORT).show();
            }
        } catch (IOException e) {
            Log.e("LoginError", "Error: " + e.getMessage());
            Toast.makeText(this, "Login failed. Try again.", Toast.LENGTH_SHORT).show();
        }
    }

    private String sendRequest(String address) throws IOException {
        URL url = new URL(address);
        HttpURLConnection connection = (HttpURLConnection) url.openConnection();
        connection.setRequestMethod("GET");
        connection.setConnectTimeout(5000);  // 超时时间为5秒
        connection.setReadTimeout(5000);

        int responseCode = connection.getResponseCode();
        responseCode2 = responseCode;
        Log.i("ServerCheck", "Response code: " + responseCode);

        if (responseCode == 200 || responseCode == 201 || responseCode == 202 || responseCode == 203) {
            BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
            StringBuilder response = new StringBuilder();
            String line;

            while ((line = reader.readLine()) != null) {
                response.append(line);
            }
            reader.close();
            connection.disconnect();
            return response.toString();
        }

        connection.disconnect();
        return null;
    }

    private void promptForServerAddress() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Enter Server Address");
        builder.setCancelable(false);  // 禁止取消

        final EditText input = new EditText(this);
        input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI);
        builder.setView(input);

        builder.setPositiveButton("Connect", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                String enteredAddress = input.getText().toString().trim();
                serverAddress = validateAddress(enteredAddress);

                if (isServerReachable(serverAddress)) {
                    if (responseCode == 200 || responseCode == 201 || responseCode == 202 || responseCode == 203) {
                        Toast.makeText(MainActivity.this, "Connected successfully!", Toast.LENGTH_SHORT).show();
                    }
                } else {
                    Toast.makeText(MainActivity.this, "Connection failed. Try again.", Toast.LENGTH_SHORT).show();
                    promptForServerAddress();  // 重新弹窗直到成功连接
                }
            }
        });

        builder.show();
    }

    private String validateAddress(String address) {
        if (!address.startsWith("http://") && !address.startsWith("https://")) {
            return "http://" + address;
        }
        return address;
    }

    private boolean isServerReachable(String address) {
        try {
            URL url = new URL(address);
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(5000);  // 超时时间为5秒
            connection.setReadTimeout(5000);
            connection.connect();

            responseCode = connection.getResponseCode();
            Log.i("ServerCheck", "Response code: " + responseCode);
            connection.disconnect();

            return responseCode == 200 || responseCode == 201 || responseCode == 202 || responseCode == 203;
        } catch (IOException e) {
            Log.e("ServerCheck", "Error checking server: " + e.getMessage());
            return false;
        }
    }

    private void showAlert(String title, String message) {
        new AlertDialog.Builder(this)
                .setTitle(title)
                .setMessage(message)
                .setPositiveButton("OK", null)
                .show();
    }

    public native String getPassticket();
}
