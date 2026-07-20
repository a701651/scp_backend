import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  vus: 400,            // 400 个虚拟用户（并发）
  duration: '30s',     // 持续 30 秒
};

export default function () {
  const payload = JSON.stringify({
    username: 'testuser',   // 改成你测试用的用户名
    password: '123456',     // 改成对应密码
  });
  const params = {
    headers: { 'Content-Type': 'application/json' },
  };
  const res = http.post('http://127.0.0.1:3000/login', payload, params);
  check(res, { 'status was 200': (r) => r.status === 200 });
  // 模拟用户思考时间，避免压测机过载（可酌情调整）
  sleep(0.1);
}