<template>
  <div class="reset-view">
    <el-form
      ref="formRef"
      :model="form"
      :rules="rules"
      label-position="top"
      @keyup.enter="handleReset"
    >
      <el-form-item label="邮箱" prop="email">
        <el-input
          v-model="form.email"
          placeholder="请输入注册邮箱"
          :prefix-icon="Message"
          size="large"
        />
      </el-form-item>

      <el-form-item label="验证码" prop="code">
        <div class="code-row">
          <el-input
            v-model="form.code"
            placeholder="请输入4位验证码"
            :prefix-icon="Key"
            size="large"
          />
          <el-button
            size="large"
            :disabled="countdown > 0"
            @click="handleSendCode"
          >
            {{ countdown > 0 ? `${countdown}s` : '发送验证码' }}
          </el-button>
        </div>
      </el-form-item>

      <el-form-item label="新密码" prop="newPassword">
        <el-input
          v-model="form.newPassword"
          type="password"
          placeholder="至少6个字符"
          :prefix-icon="Lock"
          size="large"
          show-password
        />
      </el-form-item>

      <el-form-item label="确认密码" prop="confirmPassword">
        <el-input
          v-model="form.confirmPassword"
          type="password"
          placeholder="再次输入新密码"
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
          @click="handleReset"
        >
          {{ loading ? '重置中...' : '重置密码' }}
        </el-button>
      </el-form-item>
    </el-form>

    <div class="reset-footer">
      <router-link to="/login">返回登录</router-link>
    </div>
  </div>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Message, Key, Lock } from '@element-plus/icons-vue'
import { sendVerifyCode, resetPassword } from '@/api/user'

const router = useRouter()
const formRef = ref(null)
const loading = ref(false)
const countdown = ref(0)
let timer = null

const form = reactive({
  email: '',
  code: '',
  newPassword: '',
  confirmPassword: '',
})

const validateConfirm = (rule, value, callback) => {
  if (value !== form.newPassword) {
    callback(new Error('两次输入的密码不一致'))
  } else {
    callback()
  }
}

const rules = {
  email: [
    { required: true, message: '请输入邮箱', trigger: 'blur' },
    { type: 'email', message: '请输入有效的邮箱地址', trigger: 'blur' },
  ],
  code: [
    { required: true, message: '请输入验证码', trigger: 'blur' },
    { len: 4, message: '验证码为4位数字', trigger: 'blur' },
  ],
  newPassword: [
    { required: true, message: '请输入新密码', trigger: 'blur' },
    { min: 6, message: '密码至少6个字符', trigger: 'blur' },
  ],
  confirmPassword: [
    { required: true, message: '请确认密码', trigger: 'blur' },
    { validator: validateConfirm, trigger: 'blur' },
  ],
}

async function handleSendCode() {
  if (!form.email) {
    ElMessage.warning('请先输入邮箱地址')
    return
  }
  try {
    const res = await sendVerifyCode({ email: form.email })
    if (res.error === 0) {
      ElMessage.success('验证码已发送到您的邮箱')
      countdown.value = 60
      timer = setInterval(() => {
        countdown.value--
        if (countdown.value <= 0) {
          clearInterval(timer)
        }
      }, 1000)
    } else {
      ElMessage.error(res.msg || '发送验证码失败')
    }
  } catch {
    ElMessage.error('发送验证码失败，请重试')
  }
}

async function handleReset() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  loading.value = true
  try {
    const res = await resetPassword({
      email: form.email,
      code: form.code,
      new_password: form.newPassword,
    })
    if (res.error === 0) {
      ElMessage.success('密码重置成功，请登录')
      router.push('/login')
    } else {
      ElMessage.error(res.msg || '重置密码失败')
    }
  } catch {
    ElMessage.error('重置密码失败，请重试')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.reset-view { width: 100%; }

.reset-view :deep(.el-input__wrapper:focus-within) {
  box-shadow: 0 0 0 2px var(--color-primary-bg), 0 0 0 3px var(--color-primary) !important;
}

.reset-view :deep(.el-button--large) {
  height: 44px;
  font-size: 15px;
  font-weight: 600;
  letter-spacing: 2px;
}

.code-row {
  display: flex;
  gap: 12px;
  width: 100%;
}

.code-row .el-input {
  flex: 1;
}

.code-row .el-button {
  white-space: nowrap;
  min-width: 120px;
}

.reset-footer {
  text-align: center;
  margin-top: 12px;
}

.reset-footer a {
  font-size: 13px;
  color: var(--text-secondary);
  transition: color var(--transition-fast);
}

.reset-footer a:hover { color: var(--color-primary); }
</style>
