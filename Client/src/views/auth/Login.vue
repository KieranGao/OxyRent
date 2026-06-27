<template>
  <div class="login-view">
    <el-form
      ref="formRef"
      :model="form"
      :rules="rules"
      label-position="top"
      @submit.prevent="handleLogin"
    >
      <el-form-item label="用户名" prop="username">
        <el-input
          v-model="form.username"
          placeholder="请输入用户名"
          :prefix-icon="User"
          size="large"
        />
      </el-form-item>

      <el-form-item label="密码" prop="password">
        <el-input
          v-model="form.password"
          type="password"
          placeholder="请输入密码"
          :prefix-icon="Lock"
          show-password
          size="large"
          @keyup.enter="handleLogin"
        />
      </el-form-item>

      <el-button
        type="primary"
        size="large"
        :loading="loading"
        native-type="submit"
        style="width: 100%; height: 48px; margin-top: 8px; font-size: 14px;"
      >
        登录
      </el-button>
    </el-form>

    <div class="login-footer">
      <router-link to="/reset-password">忘记密码？</router-link>
      <span class="footer-divider">|</span>
      <router-link to="/register">注册新账号</router-link>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { User, Lock } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

const router = useRouter()
const authStore = useAuthStore()
const formRef = ref(null)
const loading = ref(false)

const form = reactive({
  username: '',
  password: '',
})

const rules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, message: '密码至少6个字符', trigger: 'blur' },
  ],
}

async function handleLogin() {
  try {
    await formRef.value.validate()
  } catch {
    return
  }

  loading.value = true
  try {
    await authStore.login({ username: form.username, password: form.password })
    ElMessage.success('欢迎回来')
    router.push('/dashboard')
  } catch (err) {
    ElMessage.error(err.message || '登录失败')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-view {
  width: 100%;
}

.login-view :deep(.el-form-item) {
  margin-bottom: 20px;
}

.login-view :deep(.el-form-item__label) {
  color: var(--text-secondary);
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 1.5px;
  font-weight: 500;
  padding-bottom: 4px;
}

.login-view :deep(.el-input__wrapper) {
  height: 48px;
  background: var(--bg-primary);
  border: 1px solid var(--border);
}

.login-view :deep(.el-input__wrapper:hover) {
  border-color: var(--border-hover);
}

.login-view :deep(.el-input__wrapper.is-focus) {
  border-color: var(--accent) !important;
}

.login-view :deep(.el-input__prefix) {
  color: var(--text-tertiary);
}

.login-footer {
  text-align: center;
  margin-top: 24px;
  font-size: 13px;
  color: var(--text-secondary);
}

.login-footer a {
  color: var(--accent);
  font-weight: 500;
  transition: color 0.2s;
}

.login-footer a:hover {
  color: var(--accent-light);
}

.footer-divider {
  margin: 0 12px;
  color: var(--text-tertiary);
  font-size: 12px;
}
</style>