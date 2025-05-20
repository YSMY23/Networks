//package client;
//
//import java.io.BufferedReader;
//import java.io.InputStreamReader;
//import java.io.PrintWriter;
//import java.net.Socket;
//import java.util.Scanner;
//
//public class client {
//    public static void main(String[] args) throws Exception{
//        Socket socket = new Socket("124.223.42.84", 2525);
//        System.out.println("连接服务器！");
//        // 由套接字得到输入流
//        BufferedReader br = new BufferedReader(new InputStreamReader(socket.getInputStream()));
//        // 由套接字得到输出流
//        PrintWriter pw = new PrintWriter(socket.getOutputStream());
//        Scanner scanner = new Scanner(System.in);
//
//        // 发送HELO消息
//        System.out.println("请输入用户ID:");
//        String userId = scanner.nextLine();
//        String heloMessage = "HELO " + userId;
//        pw.println(heloMessage);
//        pw.flush();
//        String response = br.readLine();
//        System.out.println("收到服务器的消息：" + response);
//
//        if (response.contains("500 AUTH REQUIRE")) {
//            // 发送PASS消息
//            System.out.println("请输入密码:");
//            String password = scanner.nextLine();
//            String passMessage = "PASS " + password;
//            pw.println(passMessage);   //向输出流写入数据
//            pw.flush();
//            response = br.readLine();
//            System.out.println("收到服务器的消息：" + response);
//
//            if (response.contains("525 OK!")) {
//                boolean isDone = false;
//                while (!isDone) {
//                    System.out.println("请选择操作：\n1. 查询余额\n2. 取款\n3. 结束操作");
//                    int choice = scanner.nextInt();
//                    scanner.nextLine(); // 清除换行符
//                    switch (choice) {
//                        case 1:
//                            // 发送BALA消息
//                            String balaMessage = "BALA";
//                            pw.println(balaMessage);
//                            pw.flush();
//                            response = br.readLine();
//                            System.out.println("收到服务器的消息：" + response);
//                            break;
//                        case 2:
//                            // 发送WDRA消息
//                            System.out.println("请输入取款金额:");
//                            String amount = scanner.nextLine();
//                            String wdraMessage = "WDRA " + amount;
//                            pw.println(wdraMessage);
//                            pw.flush();
//                            response = br.readLine();
//                            System.out.println("收到服务器的消息：" + response);
//                            if (response.contains("525 OK")) {
//                                System.out.println("取款成功，ATM正在出钞...");
//                            } else if (response.contains("401 ERROR!")) {
//                                System.out.println("取款失败，余额不足或其他错误");
//                            }
//                            break;
//                        case 3:
//                            // 发送BYE消息
//                            String byeMessage = "BYE";
//                            pw.println(byeMessage);
//                            pw.flush();
//                            response = br.readLine();
//                            System.out.println("收到服务器的消息：" + response);
//                            isDone = true;
//                            break;
//                        default:
//                            System.out.println("无效的选择，请重新输入");
//                    }
//                }
//            }
//        }
//
//        pw.close();
//        br.close();
//        socket.close();
//        System.out.println("客户端已关闭！");
//    }
//}
import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import javax.swing.BorderFactory;

public class client {
    private JFrame frame;
    private JPanel currentPanel;
    private Socket socket;
    private BufferedReader br;
    private PrintWriter pw;
    private String userId;
    
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            try {
                new client().initialize();
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }

    private void initialize() throws Exception {
    	//创建主窗口
        frame = new JFrame("ATM Client");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(400, 300);
        frame.setLocationRelativeTo(null);

        // 连接服务器
        // socket = new Socket("124.223.42.84", 2525);
        // socket = new Socket("124.223.42.84", 2525);
        socket = new Socket("192.168.79.120", 2525);
        br = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        pw = new PrintWriter(socket.getOutputStream());
        //显示登陆面板
        showLoginPanel();
        //显示窗口
        frame.setVisible(true);
    }

    //显示登陆面板
    private void showLoginPanel() {
        if (currentPanel != null) {
            frame.remove(currentPanel);
        }

        JPanel panel = new JPanel(new GridLayout(3, 2, 5, 5));
        panel.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));
        //创建组件
        JLabel userLabel = new JLabel("User ID:");
        JTextField userField = new JTextField();
        JLabel passLabel = new JLabel("Password:");
        JPasswordField passField = new JPasswordField();
        JButton loginButton = new JButton("Login");

        //登录按钮事件处理
        loginButton.addActionListener(e -> {
            userId = userField.getText();
            String password = new String(passField.getPassword());
            
            try {
                // 发送 HELO 消息
                String heloMessage = "HELO " + userId;
                pw.println(heloMessage);
                pw.flush();
                String response = br.readLine();
                
                //服务器响应
                if (response.contains("500 AUTH REQUIRE")) {
                    // 发送 PASS 
                    String passMessage = "PASS " + password;
                    pw.println(passMessage);
                    pw.flush();
                    response = br.readLine();
                    
                    if (response.contains("525 OK!")) {
                    	//认证成功，显示主菜单
                        showMainMenuPanel();
                    } else {
                    	//认证失败，显示错误
                        JOptionPane.showMessageDialog(frame, "Authentication failed: " + response, "Error", JOptionPane.ERROR_MESSAGE);
                    }
                } else {
                    JOptionPane.showMessageDialog(frame, "Unexpected response: " + response, "Error", JOptionPane.ERROR_MESSAGE);
                }
            } catch (Exception ex) {
                ex.printStackTrace();
                JOptionPane.showMessageDialog(frame, "Communication error: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
            }
        });

        //添加组件到面板
        panel.add(userLabel);
        panel.add(userField);
        panel.add(passLabel);
        panel.add(passField);
        panel.add(new JLabel()); // Empty cell
        panel.add(loginButton);

        currentPanel = panel;
        frame.add(panel);
        frame.revalidate();
    }

    //显示主菜单面板
    private void showMainMenuPanel() {
        if (currentPanel != null) {
            frame.remove(currentPanel);
        }

        JPanel panel = new JPanel(new GridLayout(4, 1, 10, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));
        // 创建组件
        JButton balanceButton = new JButton("Check Balance");
        JButton withdrawButton = new JButton("Withdraw Money");
        JButton exitButton = new JButton("Exit");
        JLabel welcomeLabel = new JLabel("Welcome, " + userId + "!", SwingConstants.CENTER);
        // 按钮事件绑定
        balanceButton.addActionListener(e -> checkBalance());
        withdrawButton.addActionListener(e -> showWithdrawPanel());
        exitButton.addActionListener(e -> exitATM());
        // 添加组件到面板
        panel.add(welcomeLabel);
        panel.add(balanceButton);
        panel.add(withdrawButton);
        panel.add(exitButton);

        currentPanel = panel;
        frame.add(panel);
        frame.revalidate();
    }
    
     //查询余额
    private void checkBalance() {
        try {
        	//发送BALA消息
            String balaMessage = "BALA";
            pw.println(balaMessage);
            pw.flush();
            String response = br.readLine();
            //显示余额
            JOptionPane.showMessageDialog(frame, "Your balance: " + response, "Balance", JOptionPane.INFORMATION_MESSAGE);
        } catch (Exception ex) {
            ex.printStackTrace();
            JOptionPane.showMessageDialog(frame, "Error checking balance: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    //显示余额面板
    private void showWithdrawPanel() {
        if (currentPanel != null) {
            frame.remove(currentPanel);
        }

        JPanel panel = new JPanel(new BorderLayout(10, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

        JPanel inputPanel = new JPanel(new GridLayout(2, 2, 5, 5));
        JLabel amountLabel = new JLabel("Amount:");
        JTextField amountField = new JTextField();
        JButton withdrawButton = new JButton("Withdraw");
        JButton backButton = new JButton("Back");

        inputPanel.add(amountLabel);
        inputPanel.add(amountField);
        inputPanel.add(withdrawButton);
        inputPanel.add(backButton);

        withdrawButton.addActionListener(e -> {
            String amount = amountField.getText();
            try {
                String wdraMessage = "WDRA " + amount;
                pw.println(wdraMessage);
                pw.flush();
                String response = br.readLine();
                
                if (response.contains("525 OK")) {
                    JOptionPane.showMessageDialog(frame, "Withdrawal successful!\nATM is dispensing cash...", "Success", JOptionPane.INFORMATION_MESSAGE);
                    showMainMenuPanel();
                } else if (response.contains("401 ERROR!")) {
                    JOptionPane.showMessageDialog(frame, "Withdrawal failed: Insufficient balance or other error", "Error", JOptionPane.ERROR_MESSAGE);
                } else {
                    JOptionPane.showMessageDialog(frame, "Unexpected response: " + response, "Error", JOptionPane.ERROR_MESSAGE);
                }
            } catch (Exception ex) {
                ex.printStackTrace();
                JOptionPane.showMessageDialog(frame, "Error during withdrawal: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
            }
        });

        backButton.addActionListener(e -> showMainMenuPanel());

        panel.add(new JLabel("Enter withdrawal amount:", SwingConstants.CENTER), BorderLayout.NORTH);
        panel.add(inputPanel, BorderLayout.CENTER);

        currentPanel = panel;
        frame.add(panel);
        frame.revalidate();
    }

    //退出ATM系统
    private void exitATM() {
        try {
            String byeMessage = "BYE";
            pw.println(byeMessage);
            pw.flush();
            String response = br.readLine();
            JOptionPane.showMessageDialog(frame, "Goodbye! " + response, "Session Ended", JOptionPane.INFORMATION_MESSAGE);
            
            pw.close();
            br.close();
            socket.close();
            System.exit(0);
        } catch (Exception ex) {
            ex.printStackTrace();
            JOptionPane.showMessageDialog(frame, "Error closing connection: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
        }
    }
}