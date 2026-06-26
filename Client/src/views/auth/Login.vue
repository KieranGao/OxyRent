<template>
  <div class="login-view">
    <el-form
      ref="formRef"
      :model="form"
      :rules="rules"
      label-position="top"
      @keyup.enter="handleLogin"
    >
      <el-form-item label="Username" prop="username">
        <el-input
          v-model="form.username"
          placeholder="Enter your username"
          :prefix-icon="User"
          size="large"
        />
      </el-form-item>

      <el-form-item label="Password" prop="password">
        <el-input
          v-model="form.password"
          type="password"
          placeholder="Enter your password"
          :prefix-icon="Lock"
          size="large"
          show-password
        />
      </el-form-item>

      <el-form-item>
        <el-button
          type="primary"
          size="large"
          :loading="loading"
          style="width: 100%"
          @click="handleLogin"
        >
          {{ loading ? 'Logging in...' : 'Login' }}
        </el-button>
      </el-form-item>
    </el-form>

    <div class="login-footer">
      <router-link to="/register">Don't have an account? Register</router-link>
    </div>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { ElMessage } from 'element-plus'
import { User, Lock } from '@element-plus/icons-vue'

const router = useRouter()
const authStore = useAuthStore()

const formRef = ref(null)
const loading = ref(false)

const form = reactive({
  username: '',
  password: '',
})

const rules = {
  username: [
    { required: true, message: 'Please enter your username', trigger: 'blur' },
  ],
  password: [
    { required: true, message: 'Please enter your password', trigger: 'blur' },
    { min: 6, message: 'Password must be at least 6 characters', trigger: 'blur' },
  ],
}

async function handleLogin() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  loading.value = true
  try {
    await authStore.login({ username: form.username, password: form.password })
    ElMessage.success('Login successful')
    router.push('/dashboard')
  } catch (e) {
    ElMessage.error('Login failed, please check username and password')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-view { width: 100%; }

.login-view :deep(.el-input__wrapper) {
  border-radius: var(--radius-md) !important;
  transition: all var(--transition-fast) !important;
}

.login-view :deep(.el-input__wrapper:focus-within) {
  box-shadow: 0 0 0 2px var(--color-primary-bg), 0 0 0 3px var(--color-primary) !important;
}

.login-view :deep(.el-button--large) {
  height: 46px;
  font-size: 15px;
  font-weight: 600;
  letter-spacing: 2px;
  border-radius: var(--radius-md);
  margin-top: 4px;
}

.login-footer {
  text-align: center;
  margin-top: 16px;
}

.login-footer a {
  font-size: 13px;
  color: var(--text-secondary);
  transition: color var(--transition-fast);
}

.login-footer a:hover { color: var(--color-primary); }
</style>
