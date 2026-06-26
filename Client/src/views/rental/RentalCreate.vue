<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Create Rental Order</h2>
      <p>Book a vehicle for rental</p>
    </div>

    <el-card>
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
        style="max-width: 560px"
      >
        <el-form-item label="Vehicle" prop="vehicle_id">
          <el-select
            v-model="form.vehicle_id"
            placeholder="Select a vehicle"
            filterable
            style="width: 100%"
            :loading="vehiclesLoading"
          >
            <el-option
              v-for="v in vehicleOptions"
              :key="v.id"
              :label="`${v.plate_number} - ${v.brand} ${v.model}`"
              :value="v.id"
            />
          </el-select>
        </el-form-item>

        <div class="form-row">
          <el-form-item label="Start Date" prop="start_date" style="flex: 1">
            <el-date-picker
              v-model="form.start_date"
              type="date"
              placeholder="Select start date"
              value-format="YYYY-MM-DD"
              style="width: 100%"
              :disabled-date="(date) => date < new Date()"
            />
          </el-form-item>
          <el-form-item label="End Date" prop="end_date" style="flex: 1">
            <el-date-picker
              v-model="form.end_date"
              type="date"
              placeholder="Select end date"
              value-format="YYYY-MM-DD"
              style="width: 100%"
              :disabled-date="(date) => date < new Date(form.start_date || undefined)"
            />
          </el-form-item>
        </div>

        <el-form-item label="Notes">
          <el-input v-model="form.notes" type="textarea" :rows="3" placeholder="Optional notes" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            Create Order
          </el-button>
          <el-button @click="$router.back()">Cancel</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { getVehicleList } from '@/api/vehicle'
import { createRental } from '@/api/rental'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()
const formRef = ref(null)
const submitting = ref(false)
const vehiclesLoading = ref(false)
const vehicleOptions = ref([])

const form = reactive({
  vehicle_id: route.query.vehicle_id ? Number(route.query.vehicle_id) : null,
  start_date: '',
  end_date: '',
  notes: '',
})

const rules = {
  vehicle_id: [{ required: true, message: 'Please select a vehicle', trigger: 'change' }],
  start_date: [{ required: true, message: 'Please select start date', trigger: 'change' }],
  end_date: [{ required: true, message: 'Please select end date', trigger: 'change' }],
}

async function loadVehicles() {
  vehiclesLoading.value = true
  try {
    const res = await getVehicleList({ page: 1, page_size: 100, status: 'available' })
    if (res.error === 0) {
      vehicleOptions.value = res.list || res.vehicles || []
    }
  } catch {
    // ignore
  } finally {
    vehiclesLoading.value = false
  }
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    const res = await createRental({
      user_id: authStore.uid,
      vehicle_id: form.vehicle_id,
      start_date: form.start_date,
      end_date: form.end_date,
      notes: form.notes,
    })
    if (res.error === 0) {
      ElMessage.success('Rental order created')
      router.push('/rentals')
    } else {
      ElMessage.error('Failed to create order')
    }
  } catch {
    ElMessage.error('Failed to create order')
  } finally {
    submitting.value = false
  }
}

onMounted(loadVehicles)
</script>

<style scoped>
.form-row {
  display: flex;
  gap: 20px;
}
</style>
