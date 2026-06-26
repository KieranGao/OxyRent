<template>
  <div class="page-container">
    <div class="page-header">
      <h2>{{ isEdit ? 'Edit Vehicle' : 'Add Vehicle' }}</h2>
      <p>{{ isEdit ? 'Update vehicle information' : 'Add a new vehicle to inventory' }}</p>
    </div>

    <el-card v-loading="loading">
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
        style="max-width: 680px"
      >
        <el-form-item label="Plate Number" prop="plate_number">
          <el-input v-model="form.plate_number" placeholder="e.g. ABC-1234" />
        </el-form-item>

        <div class="form-row">
          <el-form-item label="Brand" prop="brand" style="flex: 1">
            <el-input v-model="form.brand" placeholder="e.g. Toyota" />
          </el-form-item>
          <el-form-item label="Model" prop="model" style="flex: 1">
            <el-input v-model="form.model" placeholder="e.g. Camry" />
          </el-form-item>
        </div>

        <div class="form-row">
          <el-form-item label="Color" prop="color" style="flex: 1">
            <el-input v-model="form.color" placeholder="e.g. White" />
          </el-form-item>
          <el-form-item label="Year" prop="year" style="flex: 1">
            <el-input-number v-model="form.year" :min="2000" :max="2030" style="width: 100%" />
          </el-form-item>
        </div>

        <div class="form-row">
          <el-form-item label="Daily Rate (CNY)" prop="daily_rate" style="flex: 1">
            <el-input-number v-model="form.daily_rate" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
          <el-form-item label="Deposit Amount (CNY)" prop="deposit_amount" style="flex: 1">
            <el-input-number v-model="form.deposit_amount" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
        </div>

        <el-form-item label="Mileage (km)">
          <el-input-number v-model="form.mileage" :min="0" style="width: 100%" />
        </el-form-item>

        <el-form-item label="Image URL">
          <el-input v-model="form.image_url" placeholder="Vehicle image URL" />
        </el-form-item>

        <el-form-item label="Description">
          <el-input v-model="form.description" type="textarea" :rows="3" placeholder="Vehicle description" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            {{ isEdit ? 'Save Changes' : 'Add Vehicle' }}
          </el-button>
          <el-button @click="$router.back()">Cancel</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { createVehicle, updateVehicle, getVehicleDetail } from '@/api/vehicle'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const isEdit = computed(() => !!route.params.id)
const loading = ref(false)
const submitting = ref(false)

const formRef = ref(null)
const form = reactive({
  plate_number: '',
  brand: '',
  model: '',
  color: '',
  year: 2024,
  mileage: 0,
  daily_rate: 0,
  deposit_amount: 0,
  image_url: '',
  description: '',
})

const rules = {
  plate_number: [{ required: true, message: 'Plate number is required', trigger: 'blur' }],
  brand: [{ required: true, message: 'Brand is required', trigger: 'blur' }],
  model: [{ required: true, message: 'Model is required', trigger: 'blur' }],
  color: [{ required: true, message: 'Color is required', trigger: 'blur' }],
  daily_rate: [{ required: true, message: 'Daily rate is required', trigger: 'blur' }],
  deposit_amount: [{ required: true, message: 'Deposit amount is required', trigger: 'blur' }],
}

async function loadVehicle() {
  if (!isEdit.value) return
  loading.value = true
  try {
    const res = await getVehicleDetail({ id: route.params.id })
    if (res.error === 0) {
      const v = res.vehicle || res
      Object.assign(form, {
        plate_number: v.plate_number || '',
        brand: v.brand || '',
        model: v.model || '',
        color: v.color || '',
        year: v.year || 2024,
        mileage: v.mileage || 0,
        daily_rate: v.daily_rate || 0,
        deposit_amount: v.deposit_amount || 0,
        image_url: v.image_url || '',
        description: v.description || '',
      })
    }
  } catch {
    ElMessage.error('Failed to load vehicle')
  } finally {
    loading.value = false
  }
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    const data = { ...form }
    if (isEdit.value) data.id = Number(route.params.id)
    const res = isEdit.value ? await updateVehicle(data) : await createVehicle(data)
    if (res.error === 0) {
      ElMessage.success(isEdit.value ? 'Vehicle updated' : 'Vehicle added')
      router.push('/vehicles')
    } else {
      ElMessage.error('Operation failed')
    }
  } catch {
    ElMessage.error('Operation failed')
  } finally {
    submitting.value = false
  }
}

onMounted(loadVehicle)
</script>

<style scoped>
.form-row {
  display: flex;
  gap: 20px;
}
</style>
