<template>
  <div class="register-view">
    <el-form
      ref="formRef"
      :model="form"
      :rules="rules"
      label-position="top"
      @keyup.enter="handleRegister"
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
          placeholder="At least 6 characters"
          :prefix-icon="Lock"
          size="large"
          show-password
        />
      </el-form-item>

      <el-form-item label="Phone" prop="phone">
        <el-input
          v-model="form.phone"
          placeholder="Enter your phone number"
          :prefix-icon="Phone"
          size="large"
        />
      </el-form-item>

      <el-form-item label="Email" prop="email">
        <el-input
          v-model="form.email"
          placeholder="Enter your email"
          :prefix-icon="Message"
          size="large"
        />
      </el-form-item>

      <el-form-item>
        <el-button
          type="primary"
          size="large"
          :loading="loading"
          style="width: 100%"
          @click="handleRegister"
        >
          {{ loading ? 'Registering...' : 'Register' }}
        </el-button>
      </el-form-item>
    </el-form>

    <div class="register-footer">
      <router-link to="/login">Already have an account? Login</router-link>
    </div>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { ElMessage } from 'element-plus'
import { User, Lock, Phone, Message } from '@element-plus/icons-vue'

const router = useRouter()
const authStore = useAuthStore()

const formRef = ref(null)
const loading = ref(false)

const form = reactive({
  username: '',
  password: '',
  phone: '',
  email: '',
})

const rules = {
  username: [
    { required: true, message: 'Please enter a username', trigger: 'blur' },
    { min: 2, max: 20, message: 'Username must be 2-20 characters', trigger: 'blur' },
  ],
  password: [
    { required: true, message: 'Please enter a password', trigger: 'blur' },
    { min: 6, message: 'Password must be at least 6 characters', trigger: 'blur' },
  ],
  phone: [
    { required: true, message: 'Please enter your phone number', trigger: 'blur' },
  ],
  email: [
    { required: true, message: 'Please enter your email', trigger: 'blur' },
    { type: 'email', message: 'Please enter a valid email', trigger: 'blur' },
  ],
}

async function handleRegister() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  loading.value = true
  try {
    await authStore.register({
      username: form.username,
      password: form.password,
      phone: form.phone,
      email: form.email,
    })
    ElMessage.success('Registration successful, please login')
    router.push('/login')
  } catch {
    ElMessage.error('Registration failed, please try again')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.register-view { width: 100%; }

.register-view :deep(.el-input__wrapper:focus-within) {
  box-shadow: 0 0 0 2px var(--color-primary-bg), 0 0 0 3px var(--color-primary) !important;
}

.register-view :deep(.el-button--large) {
  height: 44px;
  font-size: 15px;
  font-weight: 600;
  letter-spacing: 2px;
}

.register-footer {
  text-align: center;
  margin-top: 12px;
}

.register-footer a {
  font-size: 13px;
  color: var(--text-secondary);
  transition: color var(--transition-fast);
}

.register-footer a:hover { color: var(--color-primary); }
</style>
