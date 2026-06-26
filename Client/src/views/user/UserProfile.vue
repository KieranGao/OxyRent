<template>
  <div class="page-container">
    <div class="glass-card" style="max-width: 720px;" v-loading="loading">
      <div class="glass-card-header">
        <h3>个人信息</h3>
      </div>
      <div class="glass-card-body padded">
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
      >
        <el-form-item label="用户名">
          <el-input :value="authStore.username" disabled />
        </el-form-item>

        <el-form-item label="真实姓名">
          <el-input v-model="form.real_name" placeholder="请输入真实姓名" />
        </el-form-item>

        <el-form-item label="手机号">
          <el-input v-model="form.phone" placeholder="请输入手机号" />
        </el-form-item>

        <el-form-item label="邮箱">
          <el-input v-model="form.email" placeholder="请输入邮箱" />
        </el-form-item>

        <el-form-item label="身份证号">
          <el-input v-model="form.id_card" placeholder="请输入身份证号" />
        </el-form-item>

        <el-form-item label="驾驶证号">
          <el-input v-model="form.driver_license" placeholder="请输入驾驶证号" />
        </el-form-item>

        <div class="form-row">
          <el-form-item label="性别" style="flex: 1">
            <el-select v-model="form.gender" placeholder="请选择性别" style="width: 100%">
              <el-option label="男" value="male" />
              <el-option label="女" value="female" />
              <el-option label="其他" value="other" />
            </el-select>
          </el-form-item>
          <el-form-item label="出生日期" style="flex: 1">
            <el-date-picker
              v-model="form.birth_date"
              type="date"
              placeholder="请选择出生日期"
              value-format="YYYY-MM-DD"
              style="width: 100%"
            />
          </el-form-item>
        </div>

        <el-form-item label="地址">
          <el-input v-model="form.address" placeholder="请输入地址" />
        </el-form-item>

        <el-form-item label="头像URL">
          <el-input v-model="form.avatar_url" placeholder="头像图片URL" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="saving" @click="handleSave">
            保存修改
          </el-button>
        </el-form-item>
      </el-form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { getProfile, updateProfile } from '@/api/user'
import { ElMessage } from 'element-plus'

const authStore = useAuthStore()
const loading = ref(false)
const saving = ref(false)
const formRef = ref(null)

const form = reactive({
  real_name: '',
  phone: '',
  email: '',
  id_card: '',
  driver_license: '',
  gender: '',
  birth_date: '',
  address: '',
  avatar_url: '',
})

const rules = {}

async function loadProfile() {
  loading.value = true
  try {
    const res = await getProfile()
    if (res.error === 0) {
      const p = res.profile || res.user || res
      Object.assign(form, {
        real_name: p.real_name || '',
        phone: p.phone || '',
        email: p.email || '',
        id_card: p.id_card || '',
        driver_license: p.driver_license || '',
        gender: p.gender || '',
        birth_date: p.birth_date || '',
        address: p.address || '',
        avatar_url: p.avatar_url || '',
      })
    }
  } catch {
    ElMessage.error('加载个人信息失败')
  } finally {
    loading.value = false
  }
}

async function handleSave() {
  saving.value = true
  try {
    const res = await updateProfile(form)
    if (res.error === 0) {
      ElMessage.success('个人信息已更新')
    } else {
      ElMessage.error('更新个人信息失败')
    }
  } catch {
    ElMessage.error('更新个人信息失败')
  } finally {
    saving.value = false
  }
}

onMounted(loadProfile)
</script>

<style scoped>
.form-row {
  display: flex;
  gap: 20px;
}
</style>
