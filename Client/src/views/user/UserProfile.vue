<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Profile</h2>
      <p>Manage your account information</p>
    </div>

    <el-card v-loading="loading">
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
        style="max-width: 560px"
      >
        <el-form-item label="Username">
          <el-input :value="authStore.username" disabled />
        </el-form-item>

        <el-form-item label="Real Name">
          <el-input v-model="form.real_name" placeholder="Enter your real name" />
        </el-form-item>

        <el-form-item label="Phone">
          <el-input v-model="form.phone" placeholder="Enter your phone number" />
        </el-form-item>

        <el-form-item label="Email">
          <el-input v-model="form.email" placeholder="Enter your email" />
        </el-form-item>

        <el-form-item label="ID Card">
          <el-input v-model="form.id_card" placeholder="Enter your ID card number" />
        </el-form-item>

        <el-form-item label="Driver License">
          <el-input v-model="form.driver_license" placeholder="Enter your driver license number" />
        </el-form-item>

        <div class="form-row">
          <el-form-item label="Gender" style="flex: 1">
            <el-select v-model="form.gender" placeholder="Select gender" style="width: 100%">
              <el-option label="Male" value="male" />
              <el-option label="Female" value="female" />
              <el-option label="Other" value="other" />
            </el-select>
          </el-form-item>
          <el-form-item label="Birth Date" style="flex: 1">
            <el-date-picker
              v-model="form.birth_date"
              type="date"
              placeholder="Select birth date"
              value-format="YYYY-MM-DD"
              style="width: 100%"
            />
          </el-form-item>
        </div>

        <el-form-item label="Address">
          <el-input v-model="form.address" placeholder="Enter your address" />
        </el-form-item>

        <el-form-item label="Avatar URL">
          <el-input v-model="form.avatar_url" placeholder="Avatar image URL" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="saving" @click="handleSave">
            Save Changes
          </el-button>
        </el-form-item>
      </el-form>
    </el-card>
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
    ElMessage.error('Failed to load profile')
  } finally {
    loading.value = false
  }
}

async function handleSave() {
  saving.value = true
  try {
    const res = await updateProfile(form)
    if (res.error === 0) {
      ElMessage.success('Profile updated')
    } else {
      ElMessage.error('Failed to update profile')
    }
  } catch {
    ElMessage.error('Failed to update profile')
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
